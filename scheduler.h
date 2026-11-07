#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"

// Time-slice sugerido para o RR (tarefa opcional 5).
#define TIME_SLICE_MS 500

// Escalonadores disponíveis.
// TODO (opcional, tarefa 5): acrescentar SCHED_RR.
typedef enum {
    SCHED_FIFO = 0,
    SCHED_RR
} sched_algo_en;

int  set_sched_algo(const char *name);   // -1 se o nome for inválido / não implementado
const char *get_sched_algo_str(void);

/**
 * @brief One scheduling step (called once per simulator tick).
 *
 * @param current_time_ms  current simulated time
 * @param rq   ready queue
 * @param cq   command queue (finished bursts go here to wait for the next instruction)
 * @param cpu_task  in/out: the PCB currently on the CPU (NULL if the CPU is idle)
 * @return 1 if a new task was placed on the CPU this step, 0 otherwise
 */
int scheduler(uint32_t current_time_ms, queue_t *rq, queue_t *cq, pcb_t **cpu_task);

#endif //SCHEDULER_H
