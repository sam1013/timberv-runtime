// See LICENSE for license details.

/* Entry point for UT syscalls */
/* 1 tab == 4 spaces! */

#define __ASSEMBLY__

#include "portmacro.h"
#include "encoding.h"
#include "trustmon.h"
#include "pmp.h"

.local SPTOP
.section	.app.Bench.secure.data.b
.align	XALIGN
.type	SPTOP, @object
.size	SPTOP, REGBYTES
SPTOP: ASMWORD 0

# Defines a wrapper for the encls function \func.
#
# Each wrapper is tagged callable by trustmon_init.c.
# It forwards the call to \func and
# cleans up caller-saved registers that might contain sensitive data.
# 
# Assumption: All caller's parameters are passed via registers, thus we
#             can safely use the stack to store 'ra'
#
.macro GEN_OCALL_WRAPPER func

.global ocall_\func
.global oret_\func
.section        .app.Bench.secure.text.b,"ax",@progbits
.align XALIGN
ocall_\func:
    /* TODO: save and clear sensitive registers */
    addi sp, sp,       -3 * REGBYTES
    /* push original return point */
    STORECT n,  ut, ra, 2 * REGBYTES(sp)
    /* push old SPTOP */
    la   t0, SPTOP
    LOADCT  ut, t0, 0(t0)
    STORECT n,  ut, t0, 1 * REGBYTES(sp)
    /* update new SPTOP */
    la   t0, SPTOP
    STORECT ut, ut, sp, 0(t0)
    /* push expected entry point oret_\func*/
    la   ra, oret_\func
    STORECT n,  ut, ra, 0 * REGBYTES(sp)
    j    \func

.align XALIGN
oret_\func:
    /* verify entry point */
    LOADCT  ut, ra,     0 * REGBYTES(sp)
    STORECT ut, n, x0,  0 * REGBYTES(sp)
    la      t0, oret_\func
    bne     ra, t0, .
    /* verify SPTOP */
    la      t0, SPTOP
    LOADCT  ut, t0, 0(t0)
    bne     sp, t0, .
    /* restore old SPTOP */
    LOADCT  ut, t1,     1 * REGBYTES(sp)
    STORECT ut, n, x0,  1 * REGBYTES(sp)
    la   t0, SPTOP
    STORECT ut, ut, t1, 0(t0)
    /* restore original return address */
    LOADCT  ut, ra,     2 * REGBYTES(sp)
    STORECT ut, n, x0,  2 * REGBYTES(sp)
    addi sp, sp,        3 * REGBYTES
    ret

.endm

GEN_OCALL_WRAPPER vTaskBenchOcall
