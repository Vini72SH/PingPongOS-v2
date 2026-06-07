// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Gerência básica do tempo.

#include "hardware/cpu.h"
#include "kernel/dispatcher.h"

extern struct task_t* current_task;

long int clock = 0;

void timer_interrupt_handler(int irq) {
    clock++;
    if (current_task->type == USER) {
        current_task->quantum--;
        if (current_task->quantum == 0) task_yield();
    }
}

void time_init() {
    hw_irq_handle(IRQ_TIMER, timer_interrupt_handler);
    hw_timer(1, 10);
}

int systime() { return clock; }
