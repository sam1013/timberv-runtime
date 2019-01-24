#/*
#    FreeRTOS V8.2.3 - Copyright (C) 2015 Real Time Engineers Ltd.
#	
#
#    ***************************************************************************
#     *                                                                       *
#     *    FreeRTOS tutorial books are available in pdf and paperback.        *
#     *    Complete, revised, and edited pdf reference manuals are also       *
#     *    available.                                                         *
#     *                                                                       *
#     *    Purchasing FreeRTOS documentation will not only help you, by       *
#     *    ensuring you get running as quickly as possible and with an        *
#     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
#     *    the FreeRTOS project to continue with its mission of providing     *
#     *    professional grade, cross platform, de facto standard solutions    *
#     *    for microcontrollers - completely free of charge!                  *
#     *                                                                       *
#     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
#     *                                                                       *
#     *    Thank you for using FreeRTOS, and thank you for your support!      *
#     *                                                                       *
#    ***************************************************************************
#
#
#    This file is part of the FreeRTOS distribution and was contributed
#    to the project by Technolution B.V. (www.technolution.nl,
#    freertos-riscv@technolution.eu) under the terms of the FreeRTOS
#    contributors license.
#
#    FreeRTOS is free software; you can redistribute it and/or modify it under
#    the terms of the GNU General Public License (version 2) as published by the
#    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
#    >>>NOTE<<< The modification to the GPL is included to allow you to
#    distribute a combined work that includes FreeRTOS without being obliged to
#    provide the source code for proprietary components outside of the FreeRTOS
#    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#    more details. You should have received a copy of the GNU General Public
#    License and the FreeRTOS license exception along with FreeRTOS; if not it
#    can be viewed here: http://www.freertos.org/a00114.html and also obtained
#    by writing to Richard Barry, contact details for whom are available on the
#    FreeRTOS WEB site.
#
#    1 tab == 4 spaces!
#
#    http://www.FreeRTOS.org - Documentation, latest information, license and
#    contact details.
#
#    http://www.SafeRTOS.com - A version that is certified for use in safety
#    critical systems.
#
#    http://www.OpenRTOS.com - Commercial support, development, porting,
#    licensing and training services.
#*/

include config.common

# Root of RISC-V tools installation. Note that we expect to find the spike
# simulator header files here under $(RISCV)/include/spike .

PROG = $(TIMBER)

TM_PRINTF ?= 1
TM_BENCH  ?= 0
TM_MEASURE ?= 1
BENCHTEST ?= B_NONE

# stack interleaving
SI ?= 1
# code hardening
CH ?= 1

FREERTOS_SOURCE_DIR	= freertos/FreeRTOS/Source
FREERTOS_SRC = \
	$(FREERTOS_SOURCE_DIR)/croutine.c \
	$(FREERTOS_SOURCE_DIR)/list.c \
	$(FREERTOS_SOURCE_DIR)/queue.c \
	$(FREERTOS_SOURCE_DIR)/tasks.c \
	$(FREERTOS_SOURCE_DIR)/timers.c \
	$(FREERTOS_SOURCE_DIR)/event_groups.c \
	$(FREERTOS_SOURCE_DIR)/portable/MemMang/heap_2.c \
	$(FREERTOS_SOURCE_DIR)/portable/Common/mpu_wrappers.c \
	$(FREERTOS_SOURCE_DIR)/string.c \
	$(FREERTOS_SOURCE_DIR)/stream_buffer.c


APP_SOURCE_DIR = src
DEMO_SOURCE_DIR = $(FREERTOS_SOURCE_DIR)/../Demo/riscv-spike/

TRUSTMON_SOURCE_DIR = trustmon

CRT0 = machine/boot.S
PORT_SRC = $(FREERTOS_SOURCE_DIR)/portable/GCC/RISCV/port.c
PORT_ASM = $(FREERTOS_SOURCE_DIR)/portable/GCC/RISCV/portasm.S

DEMO_SRC = \
	$(DEMO_SOURCE_DIR)/arch/syscalls.c \
	$(DEMO_SOURCE_DIR)/arch/clib.c \
	$(TRUSTMON_SOURCE_DIR)/trustmon_init.c \
	apps/main.c \
	apps/app1.c \
	apps/app2.c \
	apps/app_shm_owner.c \
	apps/app_shm_target.c \
	apps/enclave_api.c

ifeq ($(TM_BENCH), 1)
DEMO_SRC += \
	apps/appbench.c \
	benchmarks/coremark/core_main.c \
	benchmarks/coremark/core_list_join.c \
	benchmarks/coremark/core_matrix.c \
	benchmarks/coremark/core_state.c \
	benchmarks/coremark/core_util.c \
	benchmarks/coremark/riscv/core_portme.c
endif

DEMO_ASM = \
	apps/app1_ocall.S

ifeq ($(TM_BENCH), 1)
DEMO_ASM += \
	apps/appbench_ocall.S
endif

TRUSTMON_SRC = \
	$(TRUSTMON_SOURCE_DIR)/enclave_tm.c \
	$(TRUSTMON_SOURCE_DIR)/measure_tm.c \
	$(TRUSTMON_SOURCE_DIR)/pmp_tm.c \
	$(TRUSTMON_SOURCE_DIR)/string_tm.c \
	$(TRUSTMON_SOURCE_DIR)/track_tm.c \
	$(TRUSTMON_SOURCE_DIR)/ttcb_tm.c \
	$(TRUSTMON_SOURCE_DIR)/mini-printf_tm.c \
	$(TRUSTMON_SOURCE_DIR)/sha256_tm.c \
	$(TRUSTMON_SOURCE_DIR)/hmac_sha2_tm.c \
	$(TRUSTMON_SOURCE_DIR)/untrusted.c


TRUSTMON_ASM = \
	$(TRUSTMON_SOURCE_DIR)/pmp_ack_tm.S \
	$(TRUSTMON_SOURCE_DIR)/encls_wrapper_tm.S \
	$(TRUSTMON_SOURCE_DIR)/mentry_tm.S

INCLUDES = \
	-I./trustmon \
	-I./conf \
	-I$(DEMO_SOURCE_DIR) \
	-I$(DEMO_SOURCE_DIR)/arch \
	-I$(DEMO_SOURCE_DIR)/include \
	-I$(DEMO_SOURCE_DIR)/../Common/include \
	-I$(RISCV)/include/spike \
	-I$(FREERTOS_SOURCE_DIR)/include \
	-I$(FREERTOS_SOURCE_DIR)/portable/GCC/RISCV \
	-Ibenchmarks/coremark \
	-Ibenchmarks/coremark/riscv

WARNINGS= -Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wsign-compare \
		-Waggregate-return -Wno-unused-variable -Wno-implicit-function-declaration 

# PERFORMANCE_RUN and ITERATIONS are coremark parameters
CFLAGS = \
	-march=$(RISCV_ISA) \
	$(WARNINGS) $(INCLUDES) \
	-fomit-frame-pointer -fno-strict-aliasing -fno-builtin -ffreestanding -nostartfiles -nostdlib \
	-D__gracefulExit -mcmodel=medany -fPIC -g \
	-O1 -DTM_BENCH=${TM_BENCH} -D${BENCHTEST}=1 -DTM_PRINTF=${TM_PRINTF} -DTM_MEASURE=${TM_MEASURE} \
	-DPERFORMANCE_RUN -DITERATIONS=1 

# -flto on individual files and -fwhole-program on final compilation unit
# instead of -fwhole-program, use -fuse-linker-plugin

LDFILE = link.ld
LDFILE_TM = link_tm.ld

GCCVER 	= $(shell $(GCC) --version | grep gcc | cut -d" " -f9)

LDFLAGS	 = -nostartfiles -static -nostdlib
LIBS	 = -L$(CCPATH)/lib/gcc/$(TARGET)/$(GCCVER) \
		   -L$(CCPATH)/$(TARGET)/lib \
		   -lc -lgcc

#
# Define all object files.
#
RTOS_OBJ = $(FREERTOS_SRC:.c=.o)
PORT_OBJ = $(PORT_SRC:.c=.o)
DEMO_OBJ = $(DEMO_SRC:.c=.o)

TRUSTMON_OBJ = $(TRUSTMON_SRC:.c=.o)
PORT_ASM_OBJ = $(PORT_ASM:.S=.o)
DEMO_ASM_OBJ = $(DEMO_ASM:.S=.o)
CRT0_OBJ = $(CRT0:.S=.o)
TRUSTMON_ASM_OBJ = $(TRUSTMON_ASM:.S=.o)
COBJS = $(PORT_OBJ) $(RTOS_OBJ) $(TRUSTMON_OBJ) $(DEMO_OBJ)
CASM  = $(COBJS:.o=.s)
AOBJS = $(CRT0_OBJ) $(PORT_ASM_OBJ) $(TRUSTMON_ASM_OBJ) $(DEMO_ASM_OBJ)
OBJS = $(AOBJS) $(COBJS)

%_tm.o: %_tm.s
	@echo "    AS TM $<"
	@cp $< $<.orig
	@# trustmon only works with SI and CH enabled, since we use inline assembler with tag-checked instructions
	@awk -v RISCV_XLEN=$(RISCV_XLEN) -v STAG=st -v SI=1 -v CH=1 -f patch.awk $<.orig > $<
	@$(GCC) -c $(CFLAGS) -o $@ $<

%.o: %.S
	@echo "    CC $<"
	@$(GCC) -c $(CFLAGS) -o $@ $<

apps/app%.o: apps/app%.s
	@echo "    AS APP $<"
	@#./patch_secure_only.sh $< ut || echo "       Patched $<"
	@cp $< $<.orig
	@awk -v RISCV_XLEN=$(RISCV_XLEN) -v STAG=ut -v mixed=1 -v SI=$(SI) -v CH=$(CH) -f patch.awk $<.orig > $<
	@$(GCC) -c $(CFLAGS) -o $@ $<

# Add stack interleaving only for benchmarking, hence stag and ntag=n
benchmarks/coremark/%.o: benchmarks/coremark/%.s
	@echo "    AS COREMARK $<"
	@#./patch_secure_only.sh $< ut || echo "       Patched $<"
	@cp $< $<.orig
	@awk -v RISCV_XLEN=$(RISCV_XLEN) -v STAG=n -v NTAG=n -v SI=$(SI) -v CH=$(CH) -f patch.awk $<.orig > $<
	@$(GCC) -c $(CFLAGS) -o $@ $<

%.o: %.s
	@echo "    AS $<"
	@$(GCC) -c $(CFLAGS) -o $@ $<

%.s: %.c
	@echo "    CC $<"
	@$(GCC) -c -S $(CFLAGS) -o $@ $<

all: $(CASM) $(PROG).elf

TM.elf : $(TRUSTMON_ASM_OBJ) $(TRUSTMON_OBJ)
	@echo Linking TM....
	@$(GCC) -o $@ -T $(LDFILE_TM) $(LDFLAGS) -r $^
	# To avoid multiple definitions of libc symbols
	@$(OBJCOPY) --redefine-syms=rename_tm.lst $@

$(PROG).elf  : $(CRT0_OBJ) $(PORT_ASM_OBJ) $(PORT_OBJ) $(RTOS_OBJ) $(DEMO_ASM_OBJ) $(DEMO_OBJ) TM.elf 
	@echo Linking Final....
	@$(GCC) -o $@ -T $(LDFILE) $(LDFLAGS) $^ $(LIBS)
	@$(OBJDUMP) -S $(PROG).elf > $(PROG).asm
	@echo Completed $@

clean :
	@rm -f $(OBJS)
	@rm -f $(OBJS:.o=.s)
	@rm -f $(OBJS:.o=.s.backup)
	@rm -f $(OBJS:.o=.s.orig)
	@rm -f $(PROG).elf 
	@rm -f $(PROG).map
	@rm -f $(PROG).asm
	@rm -f TM.elf

force_true:
	@true

#-------------------------------------------------------------
sim: all
	spike $(PROG).elf


