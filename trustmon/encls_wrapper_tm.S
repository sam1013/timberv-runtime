// See LICENSE for license details.

/* Entry point for ENCLS calls from SN */
/* 1 tab == 4 spaces! */

#define __ASSEMBLY__

#include "portmacro.h"
#include "encoding.h"
#include "trustmon.h"
#include "pmp.h"
#include "bench.h"

/* Defines a wrapper for the encls function \func.
 *
 * Each wrapper is tagged callable by trustmon_init.c.
 * It forwards the call to \func and
 * cleans up caller-saved registers that might contain sensitive data.
 * 
 * Assumption: All caller's parameters are passed via registers, thus we
 *             can safely use the stack to store 'ra'
 */
.macro GEN_ENCLS_WRAPPER func

.global tm_\func
.align 3
tm_\func:
    addi sp, sp, -REGBYTES
    STORECT NTAG, STAG, ra, 0x0(sp)
    jal  \func
    LOADSTAG ra, 0x0(sp)
    STORECT STAG, NTAG, x0, 0x0(sp)
    mv   t0, x0
    mv   t1, x0
    mv   t2, x0
    mv   t3, x0
    mv   t4, x0
    mv   t5, x0
    mv   t6, x0
    mv   a1, x0
    mv   a2, x0
    mv   a3, x0
    mv   a4, x0
    mv   a5, x0
    mv   a6, x0
    mv   a7, x0
    addi sp, sp, REGBYTES
    ret

.endm

GEN_ENCLS_WRAPPER encls_create_enclave
GEN_ENCLS_WRAPPER encls_add_enclave_region
GEN_ENCLS_WRAPPER encls_add_data
GEN_ENCLS_WRAPPER encls_add_entries
GEN_ENCLS_WRAPPER encls_init_enclave
GEN_ENCLS_WRAPPER encls_load_enclave
GEN_ENCLS_WRAPPER encls_destroy_enclave


#if TM_BENCH == 1

encls_bench:
  // BENCH_STOP(B_ENCLS); /* for benchmarking only */
#if B_ENCLS == 1
  csrrsi	t0, trace, 2
#endif
  // BENCH_START(B_ENCLSR); /* for benchmarking ENCLS return only */
#if B_ENCLSR == 1
  csrrsi	t0, trace, 1
#endif
  ret

.global enclu_bench
enclu_bench:
  // BENCH_STOP(B_ENCLU); /* for benchmarking only */
#if B_ENCLU == 1
  csrrsi	t0, trace, 2
#endif
  // BENCH_START(B_ENCLUR); /* for benchmarking ENCLU return only */
#if B_ENCLUR == 1
  csrrsi	t0, trace, 1
#endif
  li a0, 1
  ret

GEN_ENCLS_WRAPPER encls_bench

#endif // TM_BENCH == 1
