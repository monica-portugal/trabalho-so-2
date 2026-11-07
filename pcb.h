//
// Created by Martijn Kuipers on 20/10/2025.
//

#ifndef PCB_H
#define PCB_H
#include <stdint.h>

#include "msg.h"
#include "virtmem_types.h"

typedef enum  {
    TASK_COMMAND = 0,   // Task has connected and is waiting for instructions
    TASK_BLOCKED,       // Task is blocked (waiting/IO wait)
    TASK_RUNNING,       // Task is in the ready queue or currently running
    TASK_STOPPED,       // Task has finished execution (sent DONE), waiting for more messages
    TASK_TERMINATED,    // Task has been terminated and will be removed
} task_status_en;

// Define the Process Control Block (PCB) structure
typedef struct pcb_st{
    int32_t pid;                   // Process ID
    task_status_en status;         // Current status of the task defined by the pcb
    uint32_t time_ms;              // Time requested by application in milliseconds
    uint32_t ellapsed_time_ms;     // Time ellapsed since start in milliseconds
    uint32_t slice_start_ms;       // Time when the current time slice started
    uint32_t sockfd;               // Socket file descriptor for communication with the application
    uint32_t last_update_time_ms;  // Last time the PCB was updated

    page_info_t requested_pages;   // Pages requested by the application in the current burst
    page_table_t page_table;       // Page table of the application
} pcb_t;

#endif //PCB_H
