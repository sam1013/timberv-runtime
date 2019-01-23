// See LICENSE for license details.

#ifndef ENCLAVE_H
#define ENCLAVE_H

#include "trustmon.h"
#include "ttcb.h"

#define ENCLU_GETKEY    1
#define ENCLU_SHMOFFER  2
#define ENCLU_SHMACCEPT 3
#define ENCLU_SHMCLOSE  4
#if TM_BENCH == 1
#define ENCLU_BENCH  5
#endif

#define SHM_ACCESS_R  (1<<0)
#define SHM_ACCESS_W  (1<<1)
#define SHM_ACCESS_X  (1<<2)
#define SHM_ACCESS_UT (1<<3)

#define enclu(num, arg1, arg2, arg3) ({ \
  register long reg0 asm("a0") = (long)num;   \
  register long reg1 asm("a1") = (long)arg1;  \
  register long reg2 asm("a2") = (long)arg2;  \
  register long reg3 asm("a3") = (long)arg3;  \
  asm volatile("ecall\n" : "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3) \
  /* We treat enclu syscalls like ordinary function calls. Thus, we add all  */ \
  /* caller-saved registers, which are not already in the input/output operands, to the clobber list */ \
  :: "ra", "t0", "t1", "t2", "t3", "t4", "t5", "t6", \
    "a4", "a5", "a6", "a7", "memory"); \
  reg0; })

void init_crypto();
CALLABLE void tm_reset_vector();

CALLABLE tm_errcode_t tm_encls_create_enclave(ttcb_t* ttcb);
CALLABLE tm_errcode_t tm_encls_add_enclave_region(ttcb_t* ttcb, void* start, void* end, uword_t flags, region_id_t idx);
CALLABLE tm_errcode_t tm_encls_add_data(ttcb_t* ttcb, void* start, void* end);
CALLABLE tm_errcode_t tm_encls_add_entries(ttcb_t* ttcb, void* entries[]);
CALLABLE tm_errcode_t tm_encls_init_enclave(ttcb_t* ttcb);
CALLABLE tm_errcode_t tm_encls_load_enclave(ttcb_t* ttcb);
extern CALLABLE tm_errcode_t tm_encls_resume_enclave();
CALLABLE tm_errcode_t tm_encls_destroy_enclave(ttcb_t* ttcb);

CALLABLE tm_errcode_t tm_open_shm(ttcb_t* ttcb);
CALLABLE tm_errcode_t tm_close_shm(ttcb_t* ttcb);

#endif
