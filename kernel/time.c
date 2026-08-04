// PingPongOS - PingPong Operating System

// Gerência básica do tempo.

#include "kernel/time.h"

#include "hardware/cpu.h"
#include "kernel/dispatcher.h"

extern struct task_t* current_task;

long int clock = 0;

void time_handler(int irq) {
    clock++;
    if (current_task != NULL) {
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

int systime() { return clock; }
