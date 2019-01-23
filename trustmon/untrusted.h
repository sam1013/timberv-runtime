// See LICENSE for license details.

#ifndef UNTRUSTED_H
#define UNTRUSTED_H

#ifndef TM_PRINTF
#error "Include trustmon.h first, in which TM_PRINTF must be defined!"
#endif

#define TM_BUF_SIZE 128

#if TM_PRINTF == 1
int tm_printf (const char *format, ...);
void tm_printf_exit(void);
extern char normal_buf[TM_BUF_SIZE];
#endif

#endif /* UNTRUSTED_H */
