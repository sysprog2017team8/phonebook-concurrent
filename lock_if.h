#ifndef _LOCK_IF_H_
#define _LOCK_IF_H_

#include <inttypes.h>

typedef uint32_t
ptlock_t; /* change the type accorind to the lock you want to use */
#define INIT_LOCK(lock) lock_init(lock)
#define DESTROY_LOCK(lock) lock_destroy(lock)
#define LOCK(lock) lock_lock(lock)
#define UNLOCK(lock) lock_unlock(lock)

static inline
void lock_init(volatile ptlock_t *l)
{
    *l = (uint32_t)0;
}

static inline
void lock_destroy(volatile ptlock_t *l)
{
    // do nothing
}

static inline
uint32_t lock_lock(volatile ptlock_t *l)
{
    while (__sync_val_compare_and_swap(l, (uint32_t)0, (uint32_t)1) == 1);
    return 0;
}

static inline
uint32_t lock_unlock(volatile ptlock_t *l)
{
    *l = (uint32_t)0;
    return 0;
}

#endif /* _LOCK_IF_H_ */
