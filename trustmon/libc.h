// See LICENSE for license details.

#ifndef LIBC_H
#define LIBC_H

#ifndef TM_PRINTF
#error "Include trustmon.h first, in which TM_PRINTF must be defined!"
#endif

#if TM_PRINTF == 1
int tm_printf (const char *format, ...);

#define printf(...) tm_printf(__VA_ARGS__)

#define ttcb_printf(...) do { \
    tm_printf("\e[1;34m[%08x]\e[m", (uword_t)ttcb); \
    tm_printf(__VA_ARGS__); \
  } while(0)

#define enclu_printf(...) do { \
    tm_printf("\e[1;34m[%08x]\e[m", (uword_t)read_csr(sttcb)); \
    tm_printf(__VA_ARGS__); \
  } while(0)

#else
#define printf(...)
#define tm_printf(...)
#define ttcb_printf(...)
#define enclu_printf(...)
#endif

#endif /* LIBC_H */
