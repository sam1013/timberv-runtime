/*
    FreeRTOS V8.2.3 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution and was contributed
    to the project by Technolution B.V. (www.technolution.nl,
    freertos-riscv@technolution.eu) under the terms of the FreeRTOS
    contributors license.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry''s de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#define __ASSEMBLY__

#include "portmacro.h"
#include "encoding.h"
#include "bench.h"

	.align 6
	.section .text,"ax",@progbits
	.globl _mstart

/***********************************/
/*          Machine-mode           */
/***********************************/
/* Startup code */
_mstart:

#if portMACHINE_KERNEL == 1
    la		t0, trap_entry
    csrw	mtvec, t0
    li		t0, 0x1800	// MPP = 3
    csrw	mstatus, t0

#elif portSUPERVISOR_KERNEL == 1

    la		t0, mtrap_entry
    csrw	mtvec, t0
    la		t0, trap_entry
    csrw	stvec, t0
#if portUSING_TM == 1
    la		t0, st_trap_entry
    csrw	sttvec, t0
#endif
    li		t0, 0x20
    csrw	mideleg, t0
    li		t0, 0x0900	// MPP = 1, SPP = 1
    csrw	mstatus, t0

#else
  #error "Unknown kernel config"
#endif

    /* enable counter */
    csrsi	mcounteren, 0x7
    csrsi	scounteren, 0x7

#if portUSING_TM == 1
    /* enable MPU */
    li		t0, 1
    csrs	mtstatus, t0
#endif

    li	x1, 0
    li	x2, 0
    li	x3, 0
    li	x4, 0
    li	x5, 0
    li	x6, 0
    li	x7, 0
    li	x8, 0
    li	x9, 0
    li	x10, 0
    li	x11, 0
    li	x12, 0
    li	x13, 0
    li	x14, 0
    li	x15, 0
    li	x16, 0
    li	x17, 0
    li	x18, 0
    li	x19, 0
    li	x20, 0
    li	x21, 0
    li	x22, 0
    li	x23, 0
    li	x24, 0
    li	x25, 0
    li	x26, 0
    li	x27, 0
    li	x28, 0
    li	x29, 0
    li	x30, 0
    li	x31, 0

    /* initialize global pointer */
    la	gp, _gp

init_bss:
    /* init bss section */
    la	a0, __bss_start
    la	a1, __bss_end
    li	a2, 0x0
    jal	fill_block

init_sbss:
    /* init bss section */
    la	a0, __sbss_start
    la	a1, __sbss_end
    li	a2, 0x0
    jal	fill_block

write_stack_pattern:
    /* init bss section */
    la	a0, _stack_end  /* note the stack grows from top to bottom */
    la	a1, __stack
    li	a2, 0xABABABAB
    jal	fill_block

#if portUSING_MPU_WRAPPERS == 1
    /* init privileged_data section */
    la	a0, __privileged_data_start
    la	a1, __privileged_data_end
    li	a2, 0x0
    jal	fill_block
#endif

init_stack:
    /* set stack pointer */
    la	sp, _stack

#if portUSING_TM == 1
init_trustmon:
#if portTM_STACK == 1
    la	t0, _tmstack_top
    csrw stscratch, t0
#endif
    jal		tm_init
#endif

#if portMACHINE_KERNEL == 1

    /* We're already in machine mode */
    j	vPortInit

#elif portSUPERVISOR_KERNEL == 1

    /* Enable m-mode (timer) interrupt */
    /* S-mode will handle delegated interrupts */
    li		t0, 0x80	/* MPIE = 1 */
    csrs	mstatus, t0
    /* Switch to supervisor mode */
    la a0, vPortInit
    csrw mepc, a0
    mret

#else
  #error "Unknown kernel config"
#endif

/* Fills memory blocks            */
/* a0 ... start address           */
/* a1 ... end address (exclusive) */
/* a2 ... word pattern to write   */
fill_block:
    bgeu	a0, a1, fb_end
    sw		a2, 0(a0)
    addi	a0, a0, 4
    j		fill_block
fb_end:
    ret

.align 2
mtrap_entry:
	csrw	mscratch, t0
	/* Check for interrupt */
	csrr	t0, mcause
	blt	t0, x0, deleg_interrupt
	addi	t0, t0, -9 /* scall */
	bnez	t0, 1f
	li	t0, SYS_timerset
	beq	t0, a7, timer_interrupt_reset
1:
	/* clone mepc, mcause, mtval to supervisor csr's */
	csrr	t0, mepc
	csrw	sepc, t0
	/* We have an exception: copy cause/mtval directly */
	csrr	t0, mcause
	csrw	scause, t0
	csrr	t0, mtval
	csrw	stval, t0
	/* mret returns to supervisor trap_entry */
#if portUSING_TM == 1
	/* Check MTSTATUS.MO to multiplex sttvec / stvec */
	csrr	t0, mtstatus
	andi	t0, t0, TSTATUS_MO
	beqz	t0, 2f
	/* mret returns to trusted supervisor trap_entry */
	csrr	t0, sttvec
	csrw	mepc, t0
	j	3f
#endif
	/* mret returns to supervisor trap_entry */
2:	csrr	t0, stvec
	csrw	mepc, t0
3:	/* copy MPP to SPP such that supervisor mode can distinguish between ecalls from S or U */
	csrr	t0, mstatus
	srli	t0, t0, 3      /* Shift to SPP range */
	andi	t0, t0, 0x100  /* SPP field */
	csrs	mstatus, t0
	/* Set MPP = 1 to switch to supervisor mode */
	li	t0, 0x0800
	csrs	mstatus, t0
	li	t0, 0x1000
	csrc	mstatus, t0
	/* Clear SPIE and SIE and set old SIE to SPIE */
	li	t0, 0x20
	csrc	mstatus, t0
	csrr	t0, mstatus
	andi	t0, t0, 0x2
	csrc	mstatus, t0
	slli	t0, t0, 4
	csrs	mstatus, t0

	csrr	t0, mscratch
	mret
deleg_interrupt:
	/* Disable interrupt and copy MIP to SIP. This should trigger S-interrupt on mret */
	csrr	t0, mip
	andi	t0, t0, 0x80
	csrc	mie, t0
	srli	t0, t0, 2 /* shift m to s */
	csrs	mip, t0
	csrr	t0, mscratch
	mret
timer_interrupt_reset:
	li	t0, 0x20
	csrc	mip, t0
	li	t0, 0x80
	csrs	mie, t0
	/* Continue after ecall */
	csrr	t0, mepc
	addi	t0, t0, 4
	csrw	mepc, t0
	csrr	t0, mscratch
	mret


/***********************************/
/*          Kernel-mode            */
/* (can be machine or supervisor)  */
/***********************************/

/* When trap is an interrupt, this function is called */
interrupt:
	slli    t0,t0,1
	srli    t0,t0,1
	addi    t0,t0,-3
	ble	t0,x0,softwareInterrupt
	/* Interupt is timer interrupt */
	LOAD	t0, 0x0(sp)
	addi	sp, sp, REGBYTES
	j		TIMER_CMP_INT

softwareInterrupt:
	/* Interupt is software interrupt */
	LOAD  t0, 0x0(sp)
	addi  sp, sp, REGBYTES
	RET

/* For when a trap is fired */
.align 2
trap_entry:
	// BENCH_STOP(B_SYSCALL);
#if B_SYSCALL == 1
	csrrsi	t0, trace, 2
#endif
	// BENCH_STOP(B_EAEX);
#if B_EAEX == 1
	csrrsi	t0, trace, 2
#endif
	addi	sp, sp, -REGBYTES
	STORE	t0, 0x0(sp)
	/* Check for interrupt */
	csrr	t0, CAUSE
	blt	t0,x0,interrupt
	LOAD	t0, 0x0(sp)
	addi	sp, sp, REGBYTES

	/*
	 * Debug check whether interrupts are disabled
	 * either in mstatus or in mie
	 */
	csrr	t0, STATUS
	andi	t0, t0, 0xa
	beqz	t0, allok
	csrr	t0, IE
	beqz	t0, allok
	j .
	allok:

	/* System call and other traps */
	addi sp, sp, -REGBYTES*32
	STORE x1, 1*REGBYTES(sp)
	STORE x2, 2*REGBYTES(sp)
	STORE x3, 3*REGBYTES(sp)
	STORE x4, 4*REGBYTES(sp)
	STORE x5, 5*REGBYTES(sp)
	STORE x6, 6*REGBYTES(sp)
	STORE x7, 7*REGBYTES(sp)
	STORE x8, 8*REGBYTES(sp)
	STORE x9, 9*REGBYTES(sp)
	STORE x10, 10*REGBYTES(sp)
	STORE x11, 11*REGBYTES(sp)
	STORE x12, 12*REGBYTES(sp)
	STORE x13, 13*REGBYTES(sp)
	STORE x14, 14*REGBYTES(sp)
	STORE x15, 15*REGBYTES(sp)
	STORE x16, 16*REGBYTES(sp)
	STORE x17, 17*REGBYTES(sp)
	STORE x18, 18*REGBYTES(sp)
	STORE x19, 19*REGBYTES(sp)
	STORE x20, 20*REGBYTES(sp)
	STORE x21, 21*REGBYTES(sp)
	STORE x22, 22*REGBYTES(sp)
	STORE x23, 23*REGBYTES(sp)
	STORE x24, 24*REGBYTES(sp)
	STORE x25, 25*REGBYTES(sp)
	STORE x26, 26*REGBYTES(sp)
	STORE x27, 27*REGBYTES(sp)
	STORE x28, 28*REGBYTES(sp)
	STORE x29, 29*REGBYTES(sp)
	STORE x30, 30*REGBYTES(sp)
	STORE x31, 31*REGBYTES(sp)

	csrr a0, CAUSE
	csrr a1, EPC
	csrr a2, TVAL
	mv a3, sp
	jal ulPortSyscallTrap
	beqz	a0, vPrepareYield
	csrw EPC, a0

	LOAD x1, 1*REGBYTES(sp)
	LOAD x2, 2*REGBYTES(sp)
	LOAD x3, 3*REGBYTES(sp)
	LOAD x4, 4*REGBYTES(sp)
	LOAD x5, 5*REGBYTES(sp)
	LOAD x6, 6*REGBYTES(sp)
	LOAD x7, 7*REGBYTES(sp)
	LOAD x8, 8*REGBYTES(sp)
	LOAD x9, 9*REGBYTES(sp)
	LOAD x10, 10*REGBYTES(sp)
	LOAD x11, 11*REGBYTES(sp)
	LOAD x12, 12*REGBYTES(sp)
	LOAD x13, 13*REGBYTES(sp)
	LOAD x14, 14*REGBYTES(sp)
	LOAD x15, 15*REGBYTES(sp)
	LOAD x16, 16*REGBYTES(sp)
	LOAD x17, 17*REGBYTES(sp)
	LOAD x18, 18*REGBYTES(sp)
	LOAD x19, 19*REGBYTES(sp)
	LOAD x20, 20*REGBYTES(sp)
	LOAD x21, 21*REGBYTES(sp)
	LOAD x22, 22*REGBYTES(sp)
	LOAD x23, 23*REGBYTES(sp)
	LOAD x24, 24*REGBYTES(sp)
	LOAD x25, 25*REGBYTES(sp)
	LOAD x26, 26*REGBYTES(sp)
	LOAD x27, 27*REGBYTES(sp)
	LOAD x28, 28*REGBYTES(sp)
	LOAD x29, 29*REGBYTES(sp)
	LOAD x30, 30*REGBYTES(sp)
	LOAD x31, 31*REGBYTES(sp)

	addi sp, sp, REGBYTES*32

	RET

vPrepareYield:
	/* yield syscall needs special handling
	 * Instead of returning we need to switch contexts.
	 * Since stack layout of trap_entry and portasm.S macros is different,
	 * we first restore original context
	 */
	LOAD x1, 1*REGBYTES(sp)
	LOAD x2, 2*REGBYTES(sp)
	LOAD x3, 3*REGBYTES(sp)
	LOAD x4, 4*REGBYTES(sp)
	LOAD x5, 5*REGBYTES(sp)
	LOAD x6, 6*REGBYTES(sp)
	LOAD x7, 7*REGBYTES(sp)
	LOAD x8, 8*REGBYTES(sp)
	LOAD x9, 9*REGBYTES(sp)
	LOAD x10, 10*REGBYTES(sp)
	LOAD x11, 11*REGBYTES(sp)
	LOAD x12, 12*REGBYTES(sp)
	LOAD x13, 13*REGBYTES(sp)
	LOAD x14, 14*REGBYTES(sp)
	LOAD x15, 15*REGBYTES(sp)
	LOAD x16, 16*REGBYTES(sp)
	LOAD x17, 17*REGBYTES(sp)
	LOAD x18, 18*REGBYTES(sp)
	LOAD x19, 19*REGBYTES(sp)
	LOAD x20, 20*REGBYTES(sp)
	LOAD x21, 21*REGBYTES(sp)
	LOAD x22, 22*REGBYTES(sp)
	LOAD x23, 23*REGBYTES(sp)
	LOAD x24, 24*REGBYTES(sp)
	LOAD x25, 25*REGBYTES(sp)
	LOAD x26, 26*REGBYTES(sp)
	LOAD x27, 27*REGBYTES(sp)
	LOAD x28, 28*REGBYTES(sp)
	LOAD x29, 29*REGBYTES(sp)
	LOAD x30, 30*REGBYTES(sp)
	LOAD x31, 31*REGBYTES(sp)

	addi sp, sp, REGBYTES*32

	/* now context is restored and ready for yielding */
	j vPortDoYield
