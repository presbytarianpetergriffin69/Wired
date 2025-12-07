#pragma once
#include <stdint.h>
#include <stddef.h>

typedef int32_t pid_t;
typedef int32_t tid_t;

typedef enum 
{
    TASK_NEW,
    TASK_RUNNABLE,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE,
    TASK_DEAD
} task_state_t;

struct process;
struct thread;

typedef struct sched_entity 
{
    int64_t vruntime;
    int32_t weight;

    struct sched_entity *rb_left;
    struct sched_entity *rb_right;
    struct sched_entity *rb_parent;
    int rb_color;
    struct thread *owner;
} sched_entity_t;

typedef struct process
{
    pid_t pid;
    struct process *parent;

    struct thread *main_thread;
    struct thread *thread_list;

    void *addr_space;
    void *handle_table;

    uint64_t start_time_ns;
    uint64_t cpu_time_ns;
    uint32_t refcount;

    int exit_code;
    int flags;

    struct process *next_global;
} process_t;

typedef struct thread
{
    tid_t tid;
    process_t *proc;

    void *kernel_stack;
    void *user_stack;
    void arch_ctx;

    sched_entity_t se;
    task_state_t state;
    int cpu_id;

    void *wait_channel;
    int wait_result;

    struct thread *next_in_proc;
    struct thread *prev_in_proc;

    struct thread *next_wait;
    struct thread *prev_wait;
} thread_t;