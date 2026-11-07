#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>

#include "scheduler.h"
#include "virtmem.h"
#include "msg.h"
#include "queue.h"

static volatile sig_atomic_t keep_running = 1;

void handle_signal(int sig) {
    printf("\n[Signal] Caught signal %d — stopping scheduler...\n", sig);
    keep_running = 0;
}

int parse_uint_arg(const char *name, const char *val, int min, int *out) {
    char *endptr;
    errno = 0;
    long v = strtol(val, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || v < min) {
        fprintf(stderr, "Error: invalid number for %s: %s\n", name, val);
        return -1;
    }
    *out = (int)v;
    return 0;
}

int parse_args(int argc, char *argv[], int *vpages, int *pframes, int *threshold) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--vpages") == 0) {
            if (i + 1 >= argc || parse_uint_arg("--vpages", argv[++i], 1, vpages) < 0) return -1;
        } else if (strcmp(argv[i], "--pframes") == 0) {
            if (i + 1 >= argc || parse_uint_arg("--pframes", argv[++i], 1, pframes) < 0) return -1;
        } else if (strcmp(argv[i], "--threshold") == 0) {
            if (i + 1 >= argc || parse_uint_arg("--threshold", argv[++i], 0, threshold) < 0) return -1;
        } else if (strcmp(argv[i], "--vm") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --vm requires a name [FIFO, RANDOM, LRU, NRU, CLOCK]\n");
                return -1;
            }
            if (set_eviction_algo(argv[++i]) < 0) {          // B12
                fprintf(stderr, "Error: invalid / unimplemented eviction algo: %s\n", argv[i]);
                return -1;
            }
        } else if (strcmp(argv[i], "--sched") == 0) {
            if (i + 1 >= argc || set_sched_algo(argv[++i]) < 0) {
                fprintf(stderr, "Error: --sched requires FIFO (RR: tarefa opcional)\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--sched FIFO|RR] [--vpages N] [--pframes N] "
                   "[--threshold N] [--vm FIFO|RANDOM|LRU|NRU|CLOCK]\n", argv[0]);
            return 1;
        } else {
            fprintf(stderr, "Unknown option: %s\nTry --help\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

/**
 * Serve the page list of the burst that was just placed on the CPU: for each
 * virtual page, evict if needed and then request it. A negative page id means a
 * write (dirty).
 */
void handle_pages(pcb_t *task, frame_table_t *frame_table, swap_hash_t *swap,
                         int threshold) {
    for (uint32_t i = 0; i < task->requested_pages.count; i++) {
        int vpn = task->requested_pages.ids[i];
        int is_write = 0;
        if (vpn < 0) { is_write = 1; vpn = -vpn; }

        page_eviction(frame_table, swap, threshold);
        pte_t *pte = page_request(task, frame_table, swap, vpn);
        if (!pte) continue;
        pte->present = 1;
        pte->referenced = 1;
        pte->last_accessed = now_ms();
        if (is_write) pte->dirty = 1;
    }
}

int main(int argc, char *argv[]) {
    int num_vpages = 16;
    int num_pframes = 10;
    int min_pages_threshold = 3;

    int res = parse_args(argc, argv, &num_vpages, &num_pframes, &min_pages_threshold);
    if (res > 0) return EXIT_SUCCESS;
    if (res < 0) return EXIT_FAILURE;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("OSSIM: scheduler=%s, eviction=%s, vpages=%d, pframes=%d, threshold=%d\n",
           get_sched_algo_str(), get_eviction_algo_str(), num_vpages, num_pframes, min_pages_threshold);

    queue_t command_queue = {0};
    queue_t ready_queue   = {0};
    queue_t blocked_queue = {0};

    frame_table_t *frame_table = create_frame_table(num_pframes);
    if (frame_table == NULL) {                       // B14
        fprintf(stderr, "Failed to create frame table\n");
        return EXIT_FAILURE;
    }
    swap_hash_t swap = {0};

    int server_fd = setup_server_socket(SOCKET_PATH);
    if (server_fd < 0) {
        fprintf(stderr, "Failed to set up server socket\n");
        return EXIT_FAILURE;
    }
    printf("Scheduler server listening on %s...\n", SOCKET_PATH);
    reset_time();

    pcb_t *cpu_task = NULL;

    while (keep_running) {
        uint32_t now = now_ms();
        check_new_commands(&command_queue, &blocked_queue, &ready_queue, server_fd, now,
                           frame_table, &swap);
        check_blocked_queue(&blocked_queue, &command_queue, now);

        int placed = scheduler(now, &ready_queue, &command_queue, &cpu_task);
        if (placed && cpu_task) {
            handle_pages(cpu_task, frame_table, &swap, min_pages_threshold);
        }
        usleep(TICKS_MS * 1000);
    }

    printf("[Scheduler] Cleaning up and shutting down...\n");
    close(server_fd);
    unlink(SOCKET_PATH);
    destroy_swap(&swap);
    printf("[Scheduler] Shutdown complete.\n");
    return 0;
}
