#pragma once
#include <stdint.h>
#include <proc.h>

#define SCHED_DEFAULT_WEIGHT 1024

typedef struct runqueue
{
    sched_entity_t *rb_root;
    thread_t *current;
    int64_t min_vruntime;
    uint32_t nr_running;
} runqueue_t;

void sched_init(void);

void sched_init_cpu(int cpu_id);

void sched_register_thread(thread_t *t);

void sched_make_runnable(thread_t *t);

void sched_tick(int cpu_id, uint64_t delta_ns);

void sched_yield(void);

void sched_block(void *wait_channel);

void sched_wake(void *wait_channel);

void schedule(void);

void sched_thread_exit(int exit_code);

thread_t *current_thread(void);
process_t *current_process(void);