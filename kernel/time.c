// PingPongOS - PingPong Operating System

// Gerência básica do tempo.

#include "kernel/time.h"

#include "hardware/cpu.h"
#include "kernel/dispatcher.h"

long int clock = 0;

extern struct task_t* current_task;

void time_handler(int irq) {
    clock++;
    if (current_task != NULL) {
        current_task->cputime++;
        if (current_task->type == USER) {
            current_task->quantum--;
            if (current_task->quantum == 0) {
                current_task->quantum = QUANTUM;
                task_yield();
            }
        }
    }
}

void time_init() {
    hw_irq_handle(IRQ_TIMER, time_handler);
    hw_timer(1, 1);
}

void time_term() {}

unsigned int time() { return clock; }
