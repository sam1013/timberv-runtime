// See LICENSE for license details.

#ifndef MUTEX_H
#define MUTEX_H

typedef uword_t mutex_t;

#define MUTEX_TRY_LOCK(mutex_var) ({ \
  register mutex_t val = 1; \
  asm volatile ("amoswap.w.aq %0, %0, %1" : "+r"(val), "+m"(mutex_var)); \
  val;})

#define MUTEX_UNLOCK(mutex_var) ({ mutex_var = 0; })

#endif // MUTEX_H
