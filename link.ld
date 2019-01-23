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
     *    is the industry's de facto standard.                               *
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

OUTPUT_ARCH( "riscv" )

_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x1000;
_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0x1000;

WORDALIGN = 8;

/*****************************************************************************
 * Define memory layout
 ****************************************************************************/
MEMORY {
	imem : ORIGIN = 0x80000000, LENGTH = 0x00040000
	dmem : ORIGIN = 0x80040000, LENGTH = 0x00040000
}

__ROM_segment_start__ = ORIGIN( imem );
__ROM_segment_end__ = __ROM_segment_start__ + LENGTH( imem );

__RAM_segment_start__ = ORIGIN( dmem );
__RAM_segment_end__ = __RAM_segment_start__ + LENGTH( dmem );

/* Specify the default entry point to the program */

ENTRY(_mstart)

/*****************************************************************************
 * Define the sections, and where they are mapped in memory 
 ****************************************************************************/
SECTIONS {
    .machine : {
      *boot.o(.text);
    } > imem

    .tmtext :
    {
      . = ALIGN(WORDALIGN);
      _trustmon_functions_start = .;
      *(.tmtext);
      . = ALIGN(WORDALIGN) + WORDALIGN;
      _trustmon_functions_end = .;
    } > imem

    .tmdata :
    {
      . = ALIGN(WORDALIGN);
      _trustmon_data_start = .;
      *(.tmdata*);
      //. = ALIGN(WORDALIGN) + WORDALIGN;
      _trustmon_data_end = .;
    } > dmem

/*    .tmstack : {
       . = ALIGN(WORDALIGN);
       _tmstack_start = .;
       . += _STACK_SIZE;
       . = ALIGN(WORDALIGN);
       _tmstack_top = .;
       _trustmon_data_end = .;
    } > dmem */
    
    .text : {
    . = ALIGN(WORDALIGN);
      __normal_functions_start = .;
      *(.text);
      *(.text.*);
      . = ALIGN(WORDALIGN);
      __normal_functions_end = .;
    } > imem

    .enclave.text : {
      *(SORT_BY_NAME(.app.*.text.*));
    } > imem

    .init : {
        KEEP (*(.init))
    } > imem

    .fini : {
        KEEP (*(.fini))
    } > imem

    privileged_functions :
    {
      . = ALIGN(WORDALIGN);
      __privileged_functions_start = .;
      *(privileged_functions)
      *(privileged_functions.*)
      . = ALIGN(WORDALIGN);
      __privileged_functions_end = .;
    } > imem

    privileged_data :
    {
      . = ALIGN(WORDALIGN);
      __privileged_data_start = .;
      *(privileged_data)
      *(privileged_data.*)
      . = ALIGN(WORDALIGN);
      __privileged_data_end = .;
    } > dmem

    . = ALIGN(WORDALIGN);
    __normal_data_start = .;

    .data : {
      . = ALIGN(WORDALIGN);
      __data_start = .;
      *(.data)
      *(.data.*)
      *(.gnu.linkonce.d.*)
      __data_end = .;
    } > dmem

    .enclave.data : {
      *(SORT_BY_NAME(.app.*.data.*));
    } > dmem
    
    .rodata : {
       __rodata_start = .;
       *(.rodata)
       *(.rodata.*)
       *(.gnu.linkonce.r.*)
       __rodata_end = .;
    } > dmem

    .sdata : {
      _gp = . + 0x800;
      *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*)
      *(.sdata .sdata.* .gnu.linkonce.s.*)
    } > dmem

    .sbss : {
      . = ALIGN(WORDALIGN);
       __sbss_start = .;
       *(.sbss)
       *(.sbss.*)
       *(.gnu.linkonce.sb.*)
       . = ALIGN(WORDALIGN);
       __sbss_end = .;
    } > dmem

    .bss : {
       . = ALIGN(WORDALIGN);
       __bss_start = .;
       *(.bss)
       *(.bss.*)
       *(.gnu.linkonce.b.*)
       *(COMMON)
       . = ALIGN(WORDALIGN);
       __bss_end = .;
    } > dmem

    /* Generate Stack and Heap definitions */

    .heap : {
       . = ALIGN(WORDALIGN);
       _heap = .;
       _heap_start = .;
       . += _HEAP_SIZE;
       . = ALIGN(WORDALIGN);
       _heap_end = .;
    } > dmem

    . = ALIGN(WORDALIGN);
    __normal_data_end = .;

    .stack : {
       . = ALIGN(WORDALIGN);
       _stack_end = .;
       . += _STACK_SIZE;
       . = ALIGN(WORDALIGN);
       _stack = .;
       __stack = _stack;
    } > dmem
    
    _end = .;
}
