# TIMBER-V Runtime

This is a subproject of [TIMBER-V Toolchain][timberv-riscv-tools].

This repository contains the proof-of-concept implementation of the
TIMBER-V software stack. Since it evolved gradually, the naming changed
between the code and the [paper][timberv-riscv-tools], see the glossary below.

# Glossary

| Name in code | Details (Execution mode)             | Tag        | Paper        |
| -------------|--------------------------------------|------------|--------------|
| Enclave      | Trusted usermode (UT)                | T_UTRUSTED | TU           |
| App          | Normal usermode  (UN)                | T_NORMAL   | N            |
| Trustmon     | Trusted supervisor mode (ST)         | T_STRUSTED | TS (TagRoot) |
| FreeRTOS     | Normal supervisor mode, OS (SN)      | T_NORMAL   | N            |
| Machine mode | Machine mode (M)                     | any        | -            |
| encls        | Trusted OS service via TSenter       | T_CALLABLE | TC           |
| enclu        | Trusted enclave service via TSyscall | -          | -            |

# Folder structure

| Folder       | Description                                                  |
| -------------|--------------------------------------------------------------|
| `apps`       | Example apps and microbenchmarks featuring TIMBER-V enclaves |
| `freertos`   | FreeRTOS TIMBER-V port                                       |
| `conf`       | FreeRTOS configuration                                       |
| `trustmon`   | TIMBER-V trust monitor (also called TagRoot)                 |
| `machine`    | TIMBER-V machine-mode                                        |
| `benchmarks` | TIMBER-V benchmarks                                          |
| `loc`        | TIMBER-V source-code line counting                           |

# Compiling

## Preparation
Install the TIMBER-V compilers to `/opt/riscv/32` or `/opt/riscv/64`,
respectively, as described in [timberv-riscv-tools].

## Build and Run TIMBER-V
To build and run 64-bit mode:

* `export RISCV=/opt/riscv/64`
* `export PATH=/opt/riscv/64/bin:$PATH`
* `make RISCV_XLEN=64`
* `spike ./riscv64-spike.elf`

To build and run in 32-bit mode:

* `export RISCV=/opt/riscv/32`
* `export PATH=/opt/riscv/32/bin:$PATH`
* `make RISCV_XLEN=32`
* `spike --isa=RV32IMAFDC ./riscv32-spike.elf`

To debug in 64-bit mode, open three terminals and execute the following
in order:

* Terminal 1: `spike -D1 -H --rbb-port=9824 --isa=RV64IMA riscv64-spike.elf`
* Terminal 2: `openocd -f riscv-ocd.cfg`
* Terminal 3: `riscv64-unknown-elf-gdb -x run.gdb riscv64-spike.elf`

Debugging in 32-bit mode is analogous.

# Implementation Details

Here are some detailed but useful hints before diving into the source code of TIMBER-V.

## Microbenchmarks
The following environment variables control compilation of TIMBER-V:

* `TM_BENCH` Enable/disable benchmarks
* `TM_PRINTF` Enable/disable printf debug output (can affect benchmark results)
* `TM_MEASURE` Enable/disable hash measurements of TSyscalls
* `BENCHTEST=<test>` Select which benchmark should be active, e.g. `BENCHTEST=B_ECREATE`
* `SI` Enable/disable stack interleaving for applications/benchmarks
* `CH` Enable/disable code hardening for applications/benchmarks

## Compilation
Compilation is split into multiple steps:

1. Compiling source code to assembler code
2. Patching assembler code with code transformations
3. Compiling assembler code to object files

## Code transformations

Code transformations are directly applied to assembler code using the
`patch.awk` script.

The code transformations 'stack interleaving' and 'code hardening' are always
applied to trustmon.

The applications and benchmarks are transformed according to the
environment variables `SI` and `CH`.

## Linking

Any source files ending with `_tm.c` will be linked into trustmon sections,
called `.tmtext` and `.tmdata`. Both sections are bounded by the symbols
`_trustmon_start` and `_trustmon_end`. See `link_tm.ld` for details.

All trustmon object files will be linked into `TM.elf`. Since trustmon
needs its own versions of some libc functions, we rename them to avoid
naming conflicts in the final linking phase. These libc functions are
specified in `rename_tm.lst`.

Apps/enclaves are linked to the sections `.enclave.text` and `.enclave.data`.
Each application is bounded by the symbols `app_XXX_normal_text_start/end`
and `app_XXX_normal_data_start/end`, whereas their enclaves are bounded by
`app_XXX_secure_text_start/end` and `app_XXX_secure_data_start/end`.
To define code/data as secure enclave, use the `SECURE_FUNCTION` and `SECURE_DATA`
macros, which will automatically link it between the `app_XXX_secure_YYY_start/end`
symbols.

The application/enclave's name (e.g., `XXX`) must be unique. It is defined
via the macro `APPNAME` in the application's source file under `apps/`.
For example, to name the application `XXX`, use:

```
#define APPNAME XXX
#include "apps.h"
```

## Bootstrapping

### Machine mode
Initially, the machine mode boots from `machine/boot.S`.
It sets up interrupt delegation between `STVEC` (FreeRTOS) and
`STTVEC` (trustmon) and initializes the trustmon, before calling into FreeRTOS
`vPortInit`. The machine mode code is currently unprotected.

### Trustmon
The trustmon initialization will tag all memory between `_trustmon_start`
and `_trustmon_end` with `T_STRUSTED`. Furthermore, it tags trustmon entry points
(TSyscalls) with `T_CALLABLE` to make them callable from FreeRTOS.
See `trustmon/trustmon_init.c` for details.

### Enclaves
Applications can spawn enclaves during runtime via TSyscalls.
Symbols marked with `SECURE_FUNCTION` and `SECURE_DATA` are tagged with
`T_UTRUSTED` on enclave initialization. See the examples in `apps/` for details.

Whenever FreeRTOS schedules a task, it also loads corresponding enclaves
into the MPU via `tm_encls_load_enclave` in `vPortLoadTaskMPUSettings`.

# Benchmarks
Benchmarks consist of three different components:

1. Microbenchmarks
2. Coremark
3. Beebs

Each component compiles benchmark ELF binaries into `results/`, which are
then executed on TIMBER-V Spike. Execution is traced via the `TRACE` CSR, 
which prints an instruction histogram, stored in `results/BENCHMARK.hist`.
This histogram is then compressed to certain instruction classes and stored
in `results/BENCHMARK-short.hist`. Finally, the compressed histogram is
evaluated under different models (`*.model`) via `results.py`.

To run all benchmarks, execute `benchmarks/run_all.sh`.
To process the results, execute `benchmarks/results.py`.
Please note that the performance numbers depend on the compiler
version.

## Microbenchmarks
Microbenchmarks are mostly coded in `apps/appbench.c` but also in some of the
trustmon code. They are listed in `ENCLS_TESTS` and `OTHER_TESTS`
and executed from `benchmarks/compile_timberv.sh`.

## Coremark
Coremark is invoked from `apps/main.c` via `core_main`. The source is
located in `benchmarks/coremark`. The Coremark test is executed from
`benchmarks/compile_timberv.sh`.

## Beebs
Beebs benchmarks are compiled independently. They do not use the `apps`
structure nor the `Makefile`. The source is located in `benchmarks/beebs`.
Beebs is executed from `benchmarks/beebs/compile.sh`.

[timberv-riscv-tools]: https://github.com/sam1013/timberv-riscv-tools/tree/timberv
