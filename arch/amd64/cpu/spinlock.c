#include <atomic.h>
#include <spinlock.h>
#include <system.h>
#include <stdbool.h>

static inline void cpu_relax(void)
{
    __asm__ __volatile__("pause" ::: "memory");
}

void spinlock_init(spinlock_t *lock)
{
    lock->v = 0;
}

bool spin_lock(spinlock_t *lock)
{
    while (__atomic_load_n(&lock->v, __ATOMIC_RELAXED) != 0) {
        cpu_relax();
    }
}

void spin_unlock(spinlock_t *lock)
{
    __atomic_store_n(&lock->v, 0, __ATOMIC_RELEASE);
}