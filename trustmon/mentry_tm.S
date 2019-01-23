// See LICENSE for license details.

/* Entry point for UT syscalls */
/* 1 tab == 4 spaces! */

#define __ASSEMBLY__

#include "portmacro.h"
#include "encoding.h"
#include "trustmon.h"
#include "pmp.h"

.global st_trap_entry
.global tm_encls_resume_enclave

.align 6
st_trap_entry:
    csrw    stscratch, t0
#if B_EAEX == 1 || B_ERESUME == 1
    /* When benchmarking B_EAEX (almost) all ucalls act like an interrupt */
    /* We do not dispatch them to asm_enclu_dispatcher but let the trap handler store the context */
    /* Since the interrupt handler will restart the same instruction, we need to manually advance sepc to act like a syscall */
    csrr    t0, sepc
    /* Exception: We still need to properly handle ecalls from tm_encls_resume_enclave */ 
    la      t1, encls_resume_ecall
    beq     t0, t1, tm_resume_intoff
    addi    t0, t0, 4
    csrw    sepc, t0
    j enclu_interrupt
#endif
    /* Check for enclu call */
    csrr    t0, scause
    addi    t0, t0, -8 /* ucall */
    beqz    t0, asm_enclu_dispatcher
enclu_interrupt:
    addi    t0, t0, -1 /* scall */
    beqz    t0, tm_resume_intoff

    csrr    t0, scause
    //bgez    t0, st_assert     /* Sanity check: All other trap causes must be interrupts */

    /* restore t0 */
    csrr    t0, stscratch

    /* Save trusted context on stack */
    addi  sp,  sp, -REGBYTES*16
    STORECT NTAG, STAG, x1,  0  * REGBYTES(sp)
    STORECT NTAG, STAG, x2,  1  * REGBYTES(sp)
    STORECT NTAG, STAG, x3,  2  * REGBYTES(sp)
    STORECT NTAG, STAG, x4,  3  * REGBYTES(sp)
    STORECT NTAG, STAG, x5,  4  * REGBYTES(sp)
    STORECT NTAG, STAG, x6,  5  * REGBYTES(sp)
    STORECT NTAG, STAG, x7,  6  * REGBYTES(sp)
    STORECT NTAG, STAG, x8,  7  * REGBYTES(sp)
    STORECT NTAG, STAG, x9,  8  * REGBYTES(sp)
    STORECT NTAG, STAG, x10, 9  * REGBYTES(sp)
    STORECT NTAG, STAG, x11, 10 * REGBYTES(sp)
    STORECT NTAG, STAG, x12, 11 * REGBYTES(sp)
    STORECT NTAG, STAG, x13, 12 * REGBYTES(sp)
    STORECT NTAG, STAG, x14, 13 * REGBYTES(sp)
    STORECT NTAG, STAG, x15, 14 * REGBYTES(sp)
    STORECT NTAG, STAG, x16, 15 * REGBYTES(sp)
    addi  sp,  sp, -REGBYTES*15
    STORECT NTAG, STAG, x17, 0  * REGBYTES(sp)
    STORECT NTAG, STAG, x18, 1  * REGBYTES(sp)
    STORECT NTAG, STAG, x19, 2  * REGBYTES(sp)
    STORECT NTAG, STAG, x20, 3  * REGBYTES(sp)
    STORECT NTAG, STAG, x21, 4  * REGBYTES(sp)
    STORECT NTAG, STAG, x22, 5  * REGBYTES(sp)
    STORECT NTAG, STAG, x23, 6  * REGBYTES(sp)
    STORECT NTAG, STAG, x24, 7  * REGBYTES(sp)
    STORECT NTAG, STAG, x25, 8  * REGBYTES(sp)
    STORECT NTAG, STAG, x26, 9  * REGBYTES(sp)
    STORECT NTAG, STAG, x27, 10 * REGBYTES(sp)
    STORECT NTAG, STAG, x28, 11 * REGBYTES(sp)
    STORECT NTAG, STAG, x29, 12 * REGBYTES(sp)
    STORECT NTAG, STAG, x30, 13 * REGBYTES(sp)
    STORECT NTAG, STAG, x31, 14 * REGBYTES(sp)

    /* Clear register file */
    /* Store other regs to SSA */
    mv x1,  x0
    /* We do not clear x2=sp */
    mv x3,  x0
    mv x4,  x0
    mv x5,  x0
    mv x6,  x0
    mv x7,  x0
    mv x8,  x0
    mv x9,  x0
    mv x10, x0
    mv x11, x0
    mv x12, x0
    mv x13, x0
    mv x14, x0
    mv x15, x0
    mv x16, x0
#if B_EAEX == 1 || B_ERESUME == 1
    /* We need to preserve a7 = x17 for an artificial syscall from UT to SN (for benchmarking only) */
    /* Dummy operation */
    mv x0, x0
#else
    mv x17, x0
#endif
    mv x18, x0
    mv x19, x0
    mv x20, x0
    mv x21, x0
    mv x22, x0
    mv x23, x0
    mv x24, x0
    mv x25, x0
    mv x26, x0
    mv x27, x0
    mv x28, x0
    mv x29, x0
    mv x30, x0
    mv x31, x0

    /* Save metadata on stack, namely SPP | EPC | marker */
    addi sp, sp, -3 * REGBYTES

    /* Save SPP on stack and set it s.t. SN resumes in supervisor mode */
    csrr   t0, sstatus
    srli   t0, t0, 8
    andi   t0, t0, 0x1
    STORECT NTAG, STAG,  t0, 2 * REGBYTES(sp)
    li     t0, 0x100
    csrs   sstatus, t0

    /* Save EPC on stack and mask it with tm_encls_resume_enclave wrapper */
    csrr   t0, sepc
    STORECT NTAG, STAG,  t0, 1 * REGBYTES(sp)
    la     t0, tm_encls_resume_enclave
    csrw   sepc, t0

    /* Save magic on stack with tag.C to detect correct resume operations */
    li      t0, SSA_MAGIC
    STORECT NTAG, c, t0, 0 * REGBYTES(sp)

    /* Jump to normal supervisor mode for handling the trap */
    csrr    ra, stvec
    jr      ra

/* This function is callable
 * Since interrupts are enabled, we do another trap with ecall (cause = 9)
 * which jumps to tm_resume_intoff inside st_trap_entry
 */
.align REGBYTES
tm_encls_resume_enclave:
    // BENCH_START(B_ERESUME)
#if B_ERESUME == 1
	csrrsi	t0, trace, 1
#endif
encls_resume_ecall:
    ecall
    j .

/* Interrupts are disabled */
tm_resume_intoff:
    LOADCT  c, t0, 0 * REGBYTES(sp)
    STORECT c, NTAG, x0, 0 * REGBYTES(sp)
    li      t1, SSA_MAGIC
    sub     t0, t1, t0
    bnez    t0, . /* wrong magic */

    /* restore EPC */
    LOADCT  STAG, t0, 1 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 1 * REGBYTES(sp)
    csrw    sepc, t0

    /* restore SPP */
    li      t0, 0x100
    csrc    sstatus, t0
    LOADCT  STAG, t0, 2 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 2 * REGBYTES(sp)
    slli    t0, t0, 8
    csrs    sstatus, t0

    li      t0, TSTATUS_UI
    csrc    ststatus, t0

    addi  sp,  sp,  3 * REGBYTES

    /* restore register file */

    LOADCT  STAG, x17,  0 * REGBYTES(sp)
    LOADCT  STAG, x18,  1 * REGBYTES(sp)
    LOADCT  STAG, x19,  2 * REGBYTES(sp)
    LOADCT  STAG, x20,  3 * REGBYTES(sp)
    LOADCT  STAG, x21,  4 * REGBYTES(sp)
    LOADCT  STAG, x22,  5 * REGBYTES(sp)
    LOADCT  STAG, x23,  6 * REGBYTES(sp)
    LOADCT  STAG, x24,  7 * REGBYTES(sp)
    LOADCT  STAG, x25,  8 * REGBYTES(sp)
    LOADCT  STAG, x26,  9 * REGBYTES(sp)
    LOADCT  STAG, x27, 10 * REGBYTES(sp)
    LOADCT  STAG, x28, 11 * REGBYTES(sp)
    LOADCT  STAG, x29, 12 * REGBYTES(sp)
    LOADCT  STAG, x30, 13 * REGBYTES(sp)
    LOADCT  STAG, x31, 14 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  0 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  1 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  2 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  3 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  4 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  5 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  6 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  7 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  8 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  9 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 10 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 11 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 12 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 13 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 14 * REGBYTES(sp)
    addi  sp,  sp, 15 * REGBYTES

    LOADCT  STAG, x1,  0  * REGBYTES(sp)
    LOADCT  STAG, x2,  1  * REGBYTES(sp)
    LOADCT  STAG, x3,  2  * REGBYTES(sp)
    LOADCT  STAG, x4,  3  * REGBYTES(sp)
    LOADCT  STAG, x5,  4  * REGBYTES(sp)
    LOADCT  STAG, x6,  5  * REGBYTES(sp)
    LOADCT  STAG, x7,  6  * REGBYTES(sp)
    LOADCT  STAG, x8,  7  * REGBYTES(sp)
    LOADCT  STAG, x9,  8  * REGBYTES(sp)
    LOADCT  STAG, x10, 9  * REGBYTES(sp)
    LOADCT  STAG, x11, 10 * REGBYTES(sp)
    LOADCT  STAG, x12, 11 * REGBYTES(sp)
    LOADCT  STAG, x13, 12 * REGBYTES(sp)
    LOADCT  STAG, x14, 13 * REGBYTES(sp)
    LOADCT  STAG, x15, 14 * REGBYTES(sp)
    LOADCT  STAG, x16, 15 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  0 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  1 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  2 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  3 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  4 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  5 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  6 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  7 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  8 * REGBYTES(sp)
    STORECT STAG, NTAG, x0,  9 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 10 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 11 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 12 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 13 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 14 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 15 * REGBYTES(sp)
    addi  sp,  sp, 16 * REGBYTES
    sret

/* Check whether stack base (t2) and stack bound (t3) are contained inside a valid
 * PMP region number *n*, flagged with UT and ACK. If yes, jump to *found*
 */
.macro CHECK_OVERLAP n found
    csrr   t0, spmpflags\n
    andi   t0, t0, PMP_ACCESS_UTA
    xori   t0, t0, PMP_ACCESS_UTA
    bnez   t0, 1f     /* PMP range is not UT | ACK */
    csrr   t0, spmpbase\n
    blt    t2, t0, 1f /* stack base below PMP base */
    csrr   t1, spmpbound\n
    bgt    t3, t1, 1f /* stack bound above PMP bound */
    j \found
1:

.endm

/* Enclave executes in kernel mode on the same stack
 * Interrupts are disabled
 * Since enclu syscalls are treated like function calls
 * (we clobbered all caller-saved registers inside enclu(...)), 
 * we can directly use temporary registers without saving.
 */
asm_enclu_dispatcher:
    mv     t3, sp
    addi   t2, sp, -ENCLU_STACK_LIMIT
    CHECK_OVERLAP 0 check_passed
    CHECK_OVERLAP 1 check_passed
    CHECK_OVERLAP 2 check_passed
    CHECK_OVERLAP 3 check_passed
    CHECK_OVERLAP 4 check_passed
    CHECK_OVERLAP 5 check_passed
    CHECK_OVERLAP 6 check_passed
    CHECK_OVERLAP 7 check_passed
    /* Did not find matching PMP region */
    li     a0, -1 /* TM_FATAL */
    li     t0, TSTATUS_UI
    csrc   ststatus, t0
    csrr   t0, sepc
    addi   t0, t0, 4
    csrw   sepc, t0
    sret

check_passed:
    addi   sp, sp, -2 * REGBYTES
    STORECT NTAG, STAG, ra, 0 * REGBYTES(sp)
    csrr   t0, sepc
    addi   t0, t0, 4
    STORECT NTAG, STAG, t0, 1 * REGBYTES(sp)

    /* sepc is saved, we can now enable interrupts */
    csrs   sstatus, 0x2

    jal enclu_dispatcher

    /* disable interrupts before restoring sepc */
    csrc   sstatus, 0x2

    li     t0, TSTATUS_UI
    csrc   ststatus, t0
    LOADCT STAG, t0, 1 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 1 * REGBYTES(sp)
    csrw   sepc, t0
    LOADCT STAG, ra, 0 * REGBYTES(sp)
    STORECT STAG, NTAG, x0, 0 * REGBYTES(sp)
    addi   sp, sp, 2 * REGBYTES

    /* clear caller-saved registers to avoid info leak */
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
    sret

st_assert:
    csrr   a0, scause
    csrr   a1, sepc
    csrr   a2, stval
    csrr   a3, sstatus
    csrr   a4, ststatus
    csrr   a5, sttcb
    jal    st_fail
    j .
