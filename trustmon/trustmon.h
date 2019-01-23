// See LICENSE for license details.

#ifndef TRUSTMON_H
#define TRUSTMON_H

#include <encoding.h>
#include "bench.h"

#ifndef TM_PRINTF
#define TM_PRINTF 1
#endif

#ifndef TM_MEASURE
#define TM_MEASURE 1
#endif

/* ENCLU_STACK_LIMIT has to reflect maximum stack usage of any ENCLU call including one (optional) interrupt frame */
#define ENCLU_STACK_LIMIT (0x100 * REGBYTES)

#define CALLABLE __attribute__((aligned(8),noinline))

#define die(str, ...) while(1);
#ifndef assert
#define assert(x) ({ if (!(x)) die("assertion failed: %s", #x); })
#endif

#define TM_KEY_SIZE 16
#define TM_TMP_SIZE 32

#define NEW_TAG_INSNS 1

#if __riscv_xlen == 64
# define SLL32    sllw
# define STORE    sd
# define LOAD     ld
# define LWU      lwu
# define STORECT  sdct
# define LOADCT   ldct
# define LCT "ldct"
# define SCT "sdct"
# define LOG_REGBYTES 3
# define MASK_REGBYTES 0x7
# define SSA_MAGIC  0x000100010001a001ULL /* assembler "nop, nop, nop, j ." */
#else
# define SLL32    sll
# define STORE    sw
# define LOAD     lw
# define LWU      lw
# define STORECT  swct
# define LOADCT   lwct
# define LCT "lwct"
# define SCT "swct"
# define LOG_REGBYTES 2
# define MASK_REGBYTES 0x3
# define SSA_MAGIC  0x0001a001UL /* assembler "nop, j ." */
#endif

#define LOADSTAG  LOADCT STAG,
//#define LOADSTAG LOAD /* disable checked load */

// normal tag
#define NTAG n
// secure tag
#define STAG st

#ifndef __ASSEMBLY__

#include <stdint.h>

#if __riscv_xlen == 64
typedef uint64_t uword_t;
#else
typedef uint32_t uword_t;
#endif



typedef enum {
  TM_SUCCESS = 0,
  TM_INVALID,
  TM_LOCKED,
  TM_PERMISSION,
  TM_NOT_FOUND,
  TM_FATAL = -1, /* Error in TM */
} tm_errcode_t;

/**
 * Dereference pointer 'ptr' using checked load expecting 'tag'.
 * The result is returned
 */
#define DEREF_PTR_READ(ptr, tag) ({ \
  volatile uintptr_t* tmp_ptr = (uintptr_t*)(ptr); \
  volatile uintptr_t tmp_val; \
  asm volatile (LCT" "tag", %0, (%1)\n" : "=r"(tmp_val) : "r"(tmp_ptr)); \
  tmp_val; })

#define DEREF_BYTE_PTR_READ(ptr, tag) ({ \
  volatile unsigned char* tmp_ptr = (unsigned char*)(ptr); \
  volatile unsigned char tmp_val; \
  asm volatile ("lbct "tag", %0, (%1)\n" : "=r"(tmp_val) : "r"(tmp_ptr)); \
  tmp_val; })

/**
 * Dereference pointer 'ptr' using checked store expecting 'tag'
 * and storing 'val' on it.
 */
#define DEREF_PTR_WRITE(ptr, tag, val) ({ \
  volatile uintptr_t* tmp_ptr = (uintptr_t*)(ptr); \
  volatile uintptr_t tmp_val = (uintptr_t)(val); \
  asm volatile (SCT" "tag", "tag", %0, (%1)\n" : : "r"(tmp_val), "r"(tmp_ptr) : "memory"); \
  tmp_val; })

#define DEREF_BYTE_PTR_WRITE(ptr, tag, val) ({ \
  volatile unsigned char* tmp_ptr = (unsigned char*)(ptr); \
  volatile unsigned char tmp_val = (unsigned char)(val); \
  asm volatile ("sbct "tag", "tag", %0, (%1)\n" : : "r"(tmp_val), "r"(tmp_ptr) : "memory"); \
  tmp_val; })

#endif // __ASSEMBLY__
#endif
