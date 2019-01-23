// See LICENSE for license details.

#ifndef _PMP_H
#define _PMP_H

#define PMP_ACCESS_R  (1<<0)
#define PMP_ACCESS_W  (1<<1)
#define PMP_ACCESS_X  (1<<2)
#define PMP_ACCESS_UT (1<<3)
#define PMP_ACCESS_A  (1<<4)
#define PMP_ACCESS_ST (1<<5)
#define PMP_SOFT_SHM  (1<<6) /* Denotes shared memory region of other enclave */
#define PMP_ACCESS_RWX (PMP_ACCESS_R | PMP_ACCESS_W | PMP_ACCESS_X)
#define PMP_ACCESS_UTA (PMP_ACCESS_UT | PMP_ACCESS_A)
#define PMP_ACCESS_F (PMP_ACCESS_RWX | PMP_ACCESS_TA)

/* These flags can be set by untrusted code */
#define PMP_ACCESS_UMASK (PMP_ACCESS_RWX | PMP_ACCESS_UT)

#ifndef __ASSEMBLY__

#include "encoding.h"
#include <stdint.h>

typedef struct {
  uintptr_t base;
  uintptr_t bound;
  uintptr_t flags;
} pmp_entry_t;

#define SET_PMP(x, base, bound, flags) \
  do { \
    write_csr(mpmpbase##x,  (base)); \
    write_csr(mpmpbound##x, (bound)); \
    write_csr(mpmpflags##x, (flags)); \
  } while(0)

#define CLEAR_PMP(x) \
  do { \
    write_csr(mpmpbase##x,  0); \
    write_csr(mpmpbound##x, 0); \
    write_csr(mpmpflags##x, 0); \
  } while(0)

#define READ_PMP(x, entry) \
  do { \
    ((pmp_entry_t*)entry)->base = read_csr(mpmpbase##x); \
    ((pmp_entry_t*)entry)->bound = read_csr(mpmpbound##x); \
    ((pmp_entry_t*)entry)->flags = read_csr(mpmpflags##x); \
  } while(0)

#define LOAD_PMP(x, symbol, flags) \
  do { \
    write_csr(mpmpbase##x,  &_##symbol##_start); \
    write_csr(mpmpbound##x, &_##symbol##_end); \
    write_csr(mpmpflags##x, flags); \
  } while(0)

void dump_pmp();

void flush_pmp_trusted();

tm_errcode_t mpu_load_ttcb_locked(ttcb_t* ttcb);
tm_errcode_t mpu_unload_ttcb_locked(ttcb_t* ttcb);
extern tm_errcode_t asm_ack_mpu(region_t* region);
extern void asm_unack_mpu();

#endif //__ASSEMBLY__

#endif //PMP_H
