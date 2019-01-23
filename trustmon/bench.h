// See LICENSE for license details.

#ifndef BENCH_H
#define BENCH_H


#ifndef TM_BENCH
#define TM_BENCH 0
#endif

#ifndef BENCH
#define BENCH 0
#endif

#if TM_BENCH == 1
/* Start recording instruction histogram in TIMBER-V Spike */
#define BENCH_START(x) if ((x) == 1) { write_csr(trace, 1); }
/* Stop recording. This immediately terminates TIMBER-V Spike
 * and outputs the instruction histogram */
#define BENCH_STOP(x)  if ((x) == 1) { write_csr(trace, 2); }
#else
#define BENCH_START(x)
#define BENCH_STOP(x)
#endif


#ifndef B_NOP
#define B_NOP     0
#endif
#ifndef B_SYSCALL
#define B_SYSCALL 0
#endif
#ifndef B_ENCLS
#define B_ENCLS   0
#endif
#ifndef B_ENCLSR
#define B_ENCLSR  0
#endif
#ifndef B_ECREATE
#define B_ECREATE 0
#endif
#ifndef B_EADD0
#define B_EADD0   0
#endif
#ifndef B_EADD1
#define B_EADD1   0
#endif
#ifndef B_EADD2
#define B_EADD2   0
#endif
#ifndef B_EADD3
#define B_EADD3   0
#endif
#ifndef B_EDATA0
#define B_EDATA0  0
#endif
#ifndef B_EDATA1
#define B_EDATA1  0
#endif
#ifndef B_EDATA2
#define B_EDATA2  0
#endif
#ifndef B_EDATA3
#define B_EDATA3  0
#endif
#ifndef B_EENTRY0
#define B_EENTRY0 0
#endif
#ifndef B_EENTRY1
#define B_EENTRY1 0
#endif
#ifndef B_EENTRY2
#define B_EENTRY2 0
#endif
#ifndef B_EENTRY3
#define B_EENTRY3 0
#endif
#ifndef B_ELOAD
#define B_ELOAD   0
#endif
#ifndef B_EINIT
#define B_EINIT   0
#endif
#ifndef B_EDESTROY
#define B_EDESTROY   0
#endif
#ifndef B_ENCLU
#define B_ENCLU      0
#endif
#ifndef B_ENCLUR
#define B_ENCLUR     0
#endif
#ifndef B_EGETKEY
#define B_EGETKEY    0
#endif
#ifndef B_ESHMOFFER
#define B_ESHMOFFER  0
#endif
#ifndef B_ESHMACCEPT
#define B_ESHMACCEPT 0
#endif
#ifndef B_ESHMCLOSE
#define B_ESHMCLOSE  0
#endif
#ifndef B_EAEX
#define B_EAEX    0
#endif
#ifndef B_ERESUME
#define B_ERESUME 0
#endif
#ifndef B_ECALL
#define B_ECALL   0
#endif
#ifndef B_ECALLR
#define B_ECALLR  0
#endif
#ifndef B_OCALL
#define B_OCALL   0
#endif
#ifndef B_OCALLR
#define B_OCALLR  0
#endif
#ifndef B_COREMARK
#define B_COREMARK   0
#endif

#endif // BENCH_H
