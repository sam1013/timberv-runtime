// See LICENSE for license details.

/* Entry point for UT syscalls */
/* 1 tab == 4 spaces! */

#define __ASSEMBLY__

#include "portmacro.h"
#include "encoding.h"
#include "trustmon.h"
#include "pmp.h"

.global asm_ack_mpu
.global asm_unack_mpu

.macro asm_verify_mpu n, ttcb

.endm

/* 
 * Acknowledge PMP region <n> against corresponding TTCB region.
 * The PMP region is only acknowledged if its range and access flags are contained within the TTCB region.
 *
 * a0 ... points to current ttcb->region. Is incremented to the next region after this macro
 * Macro uses t0 ... t3 without saving
 * Macro stores result on t3
 * PMP region number *n*
 */
.macro CHECK_ACK_PMP_REGION n
    /* Let t0 point to correct TTCB region */
    addi   t0, a0, \n * 3 * REGBYTES

    /* We only test PMP regions with UT flag set */
    csrr   t2, spmpflags\n
    andi   t1, t2, PMP_ACCESS_UT
    beqz   t1, 2f          /* PMP range is not UT, ignore */

    /* Test PMP flags are a subset of TTCB flags */
    LOADSTAG t1, 2 * REGBYTES(t0)
    not    t1, t1
    and    t2, t2, t1
    bnez   t2, 1f          /* Error: PMP flags not a subset of TTCB flags */

    /* Test PMP base/bound */
    LOADSTAG t1, 0 * REGBYTES(t0)
    csrr   t2, spmpbase\n
    blt    t2, t1, 1f      /* Error: PMP base below TTCB base */
    LOADSTAG t1, 1 * REGBYTES(t0)
    csrr   t2, spmpbound\n
    bgt    t2, t1, 1f      /* Error: PMP bound above TTCB bound */
    
    /* Acknowledge PMP region */
    li     t1, PMP_ACCESS_A
    csrs   spmpflags\n, t1
    j 2f
1: /* Error */
    or     t3, t3, 1
2:

.endm

// a0 ... ttcb->region
// return a0 ... 0 on success, 1 on error
asm_ack_mpu:
    li    t3, 0
    CHECK_ACK_PMP_REGION 0
    CHECK_ACK_PMP_REGION 1
    CHECK_ACK_PMP_REGION 2
    CHECK_ACK_PMP_REGION 3
    CHECK_ACK_PMP_REGION 4
    CHECK_ACK_PMP_REGION 5
    CHECK_ACK_PMP_REGION 6
    CHECK_ACK_PMP_REGION 7
    mv    a0, t3
    ret

// a0 ... ttcb->region
// return a0 ... 0 on success, 1 on error
asm_unack_mpu:
    li    t0, PMP_ACCESS_A
    csrc spmpflags0, t0
    csrc spmpflags1, t0
    csrc spmpflags2, t0
    csrc spmpflags3, t0
    csrc spmpflags4, t0
    csrc spmpflags5, t0
    csrc spmpflags6, t0
    csrc spmpflags7, t0
    ret
