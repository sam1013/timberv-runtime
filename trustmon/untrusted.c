// See LICENSE for license details.

#include "trustmon.h"
#include "untrusted.h"

#if TM_PRINTF == 1

__attribute__((aligned(8))) char normal_buf[TM_BUF_SIZE];

extern int puts (const char* s);

#endif
