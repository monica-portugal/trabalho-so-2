#ifndef QUEUE_H
#define QUEUE_H
#include <stdint.h>

#define MAX_CLIENTS 128

#include "pcb.h"
#include "virtmem_types.h"

// Define singly linked list elements
typedef struct queue_elem_st queue_elem_t;

typedef struct queue_elem_st {
    pcb_t *pcb;
    struct queue_elem_st *next;
} queue_elem_t;

// Define the queue structure
// We define the head and the tail to make it easier to enqueue and dequeue
typedef struct queue_st  {
    queue_elem_t* head;
    queue_elem_t* tail;
    ssize_t size;
} queue_t;

uint32_t now_ms(void);
void reset_time(void);

/**
 * @brief Create a new pcb (process control block)
 *
 * @param pid The process ID of the task
 * @param sockfd The socket file descriptor for communication with the application
 * @param time_ms a time field (either for run or block)
 */
pcb_t *new_pcb(int32_t pid, uint32_t sockfd, uint32_t time_ms);

/**
 * @brief Enqueue a pcb into the queue (at the tail, FIFO order)
 * @return 1 on success, 0 on failure
 */
int enqueue_pcb(queue_t* q, pcb_t* task);

/**
 * @brief Dequeue a pcb from the head of the queue (FIFO order)
 * @return The pcb at the front of the queue, or NULL if the queue is empty
 */
pcb_t* dequeue_pcb(queue_t* q);

/**
 * @brief Remove a specific element from the queue.
 * Neither the element, nor the pcb inside the element, are freed.
 * @return The removed element, or NULL if the element was not found
 */
queue_elem_t *remove_queue_elem(queue_t* q, queue_elem_t* elem);

void check_blocked_queue(queue_t *blocked_queue, queue_t *command_queue, uint32_t current_time_ms);

void check_new_commands(queue_t *command_queue, queue_t *blocked_queue, queue_t *ready_queue,
                        int server_fd, uint32_t current_time_ms,
                        frame_table_t *frame_table, swap_hash_t *swap);

ssize_t receive_msg(int sockfd, void *msg, ssize_t msg_len);

int setup_server_socket(const char *socket_path);
#endif //QUEUE_H
