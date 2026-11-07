#include "queue.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "debug.h"
#include "virtmem.h"

static uint32_t PID = 0;
static uint32_t start_time_ms = 0;

void reset_time(void) {
    start_time_ms = 0;
    start_time_ms = now_ms();
}

uint32_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint32_t)(uint64_t)ts.tv_sec * 1000
         + (uint64_t)ts.tv_nsec / 1000000) - start_time_ms;
}

pcb_t *new_pcb(pid_t pid, uint32_t sockfd, uint32_t time_ms) {
    pcb_t *new_task = malloc(sizeof(pcb_t));
    if (!new_task) return NULL;

    new_task->pid = pid;
    new_task->status = TASK_COMMAND;
    new_task->slice_start_ms = 0;
    new_task->sockfd = sockfd;
    new_task->time_ms = time_ms;
    new_task->ellapsed_time_ms = 0;
    new_task->last_update_time_ms = 0;

    new_task->requested_pages.count = 0;
    for (int i = 0; i < MAX_PAGES; i++) new_task->requested_pages.ids[i] = 0;
    if (create_page_table(&new_task->page_table, MAX_PAGES) < 0) {
        free(new_task);
        return NULL;
    }
    return new_task;
}

int enqueue_pcb(queue_t* q, pcb_t* task) {
    queue_elem_t* elem = malloc(sizeof(queue_elem_t));
    if (!elem) return 0;

    elem->pcb = task;
    elem->next = NULL;

    if (q->tail) {
        q->tail->next = elem;
    } else {
        q->head = elem;
    }
    q->tail = elem;
    q->size++;
    return 1;
}

pcb_t* dequeue_pcb(queue_t* q) {
    if (!q || !q->head) return NULL;

    queue_elem_t* node = q->head;
    pcb_t* task = node->pcb;

    q->head = node->next;
    if (!q->head)
        q->tail = NULL;

    free(node);
    q->size--;
    return task;
}

queue_elem_t *remove_queue_elem(queue_t* q, queue_elem_t* elem) {
    queue_elem_t* it = q->head;
    queue_elem_t* prev = NULL;
    while (it != NULL) {
        if (it == elem) {
            if (prev) {
                prev->next = it->next;
            } else {
                q->head = it->next;
            }
            if (it == q->tail) {
                q->tail = prev;
            }
            q->size--;
            return it;
        }
        prev = it;
        it = it->next;
    }
    printf("Queue element not found in queue\n");
    return NULL;
}

/**
 * @brief Set up the server socket for the scheduler.
 *
 * Creates a UNIX domain socket, binds it to SOCKET_PATH, sets it to listen and
 * to non-blocking mode.
 *
 * @return the server file descriptor on success, or -1 on failure
 */
int setup_server_socket(const char *socket_path) {
    int server_fd;
    struct sockaddr_un addr;

    unlink(socket_path);

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags != -1) {
        if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl: set non-blocking");
        }
    }
    return server_fd;
}

/**
 * When using read() we can get partial reads, so loop until the full message arrives.
 * @return bytes read, 0 if no data available (non-blocking), -1 on error / peer closed
 */
ssize_t receive_msg(int sockfd, void *msg, ssize_t msg_len) {
    ssize_t want = msg_len;
    ssize_t off = 0;

    while (off < want) {
        ssize_t n = read(sockfd, ((char*)msg) + off, want - off);
        if (n > 0) {
            off += n;
            if (off == want) return off;
        }
        if (n == 0)  return -1; // peer closed
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
            perror("read");
            return -1;
        }
    }
    return off;
}

/**
 * @brief Accept new client connections and service the COMMAND queue.
 *
 * New connections become PCBs in the COMMAND queue. PCBs already in COMMAND are
 * polled for a RUN or BLOCK message and moved to READY / BLOCKED accordingly.
 */
void check_new_commands(queue_t *command_queue, queue_t *blocked_queue, queue_t *ready_queue,
                        int server_fd, uint32_t current_time_ms,
                        frame_table_t *frame_table, swap_hash_t *swap)
{
    // Accept new client connections
    int client_fd;
    do {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EMFILE || errno == ENFILE) {
                perror("accept: too many fds");
                break;
            }
            if (errno == EINTR)        continue;
            if (errno == ECONNABORTED) continue;
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
                perror("accept");
            }
            break;
        }
        int flags = fcntl(client_fd, F_GETFL, 0);
        if (flags != -1) {
            if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                perror("fcntl: set non-blocking");
            }
        }
        int fdflags = fcntl(client_fd, F_GETFD, 0);
        if (fdflags != -1) {
            fcntl(client_fd, F_SETFD, fdflags | FD_CLOEXEC);
        }

        DBG("[Scheduler] New client connected: fd=%d", client_fd);

        pcb_t *pcb = new_pcb(++PID, client_fd, 0);
        enqueue_pcb(command_queue, pcb);
    } while (client_fd > 0);

    // Walk the COMMAND queue looking for messages
    queue_elem_t *elem = command_queue->head;
    while (elem != NULL) {
        pcb_t *current_pcb = elem->pcb;
        msg_t msg;

        ssize_t n = receive_msg(current_pcb->sockfd, &msg, sizeof(msg_t));
        if (n == 0) {
            elem = elem->next;
            continue;
        }

        if (n < 0) {
            DBG("Connection closed by client (fd=%d)", current_pcb->sockfd);
            free_pages(frame_table, swap, current_pcb);
            free(current_pcb->page_table.ptes);
            queue_elem_t *next = elem->next;
            remove_queue_elem(command_queue, elem);
            close(current_pcb->sockfd);
            free(current_pcb);
            free(elem);
            elem = next;
            continue;
        }

        if (msg.request == PROCESS_REQUEST_RUN) {
            current_pcb->pid = msg.pid;
            current_pcb->time_ms = msg.time_ms;
            current_pcb->ellapsed_time_ms = 0;
            current_pcb->status = TASK_RUNNING;
            current_pcb->requested_pages = msg.pages;

            enqueue_pcb(ready_queue, current_pcb);

        } else if (msg.request == PROCESS_REQUEST_BLOCK) {
            current_pcb->pid = msg.pid;
            current_pcb->time_ms = msg.time_ms;
            current_pcb->status = TASK_BLOCKED;

            enqueue_pcb(blocked_queue, current_pcb);

        } else {
            printf("Unexpected message received from client\n");
            elem = elem->next;
            continue;
        }

        queue_elem_t *next = elem->next;
        remove_queue_elem(command_queue, elem);
        free(elem);
        elem = next;

        // Send ACK back to the client
        msg_t ack_msg = {
            .pid = current_pcb->pid,
            .request = PROCESS_REQUEST_ACK,
            .time_ms = current_time_ms
        };
        if (write(current_pcb->sockfd, &ack_msg, sizeof(msg_t)) != sizeof(msg_t)) {
            perror("write");
        }
    }
}

/**
 * @brief Advance PCBs in the BLOCKED queue and release the ones whose I/O is done.
 *
 * Each tick decrements the remaining block time. When it reaches zero the PCB is
 * sent a DONE message and moved to the COMMAND queue for the next instruction.
 */
void check_blocked_queue(queue_t *blocked_queue, queue_t *command_queue, uint32_t current_time_ms) {
    queue_elem_t *elem = blocked_queue->head;
    while (elem != NULL) {
        pcb_t *pcb = elem->pcb;

        if (pcb->last_update_time_ms < current_time_ms) {
            if (pcb->time_ms > TICKS_MS) {
                pcb->time_ms -= TICKS_MS;
            } else {
                pcb->time_ms = 0;
            }
        }

        if (pcb->time_ms == 0) {
            msg_t msg = {
                .pid = pcb->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write(pcb->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            pcb->status = TASK_COMMAND;
            pcb->last_update_time_ms = current_time_ms;
            enqueue_pcb(command_queue, pcb);

            remove_queue_elem(blocked_queue, elem);
            queue_elem_t *tmp = elem;
            elem = elem->next;
            free(tmp);
        } else {
            elem = elem->next;
        }
    }
}
