// See LICENSE for license details.

#ifndef _TAG_H
#define _TAG_H

#include "trustmon.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>

#if NEW_TAG_INSNS == 1

#define T_NORMAL   "n"
#define T_CALLABLE "c"
#define T_UTRUSTED "ut"
#define T_STRUSTED "st"

#define load_test_tag(addr, required_tag) ({ \
  register uword_t ltt_temp; \
  register void* ltt_addr = (void*)(addr); \
  asm volatile ("ltt "required_tag", %0, (%1)\n" : "=r"(ltt_temp) : "r"(ltt_addr)); \
  ltt_temp;})


#define check_tag_word(addr, required_tag) _check_tag_word(addr, required_tag)
#define _check_tag_word(addr, required_tag) ({ \
  register uword_t ctw_temp; \
  register void* ctw_addr = (void*)(addr); \
  asm volatile (LCT" "required_tag", %0, (%1)\n" : "=r"(ctw_temp) : "r"(ctw_addr)); \
  TM_SUCCESS;})

#define ato_check_set_tag(addr, required_tag, new_tag) _ato_check_set_tag(addr, required_tag, new_tag)
#define _ato_check_set_tag(addr, required_tag, new_tag) ({ \
  volatile register uword_t cst_temp; \
  volatile register void* cst_addr = (void*)(addr); \
  asm volatile (LCT" "required_tag", %0, (%1)\n" \
                SCT" "required_tag", "new_tag", %0, (%1)\n" \
                : "+r"(cst_temp) : "r"(cst_addr) : "memory"); \
  TM_SUCCESS;})

#define ato_check_set_tag_zero(addr, required_tag, new_tag) _ato_check_set_tag_zero(addr, required_tag, new_tag)
#define _ato_check_set_tag_zero(addr, required_tag, new_tag) ({ \
  volatile register void* cst_addr = (void*)(addr); \
  asm volatile (SCT" "required_tag", "new_tag", x0, (%0)\n" \
                : : "r"(cst_addr) : "memory"); \
  TM_SUCCESS;})

#define ato_check_set_tag_data(addr, required_tag, new_tag, data) _ato_check_set_tag_data(addr, required_tag, new_tag, data)
#define _ato_check_set_tag_data(addr, required_tag, new_tag, data) ({ \
  register uword_t cstd_temp = data; \
  register void* cstd_addr = (void*)(addr); \
  asm volatile (SCT" "required_tag", "new_tag", %0, (%1)\n" : : "r"(cstd_temp), "r" (cstd_addr)); \
  TM_SUCCESS;})

#define ato_check_set_tag_range(saddr, eaddr, required_tag, new_tag) ({ \
  tm_errcode_t cstr_ret = TM_SUCCESS; \
  for (uword_t* paddr = (uword_t*)(saddr); paddr < (uword_t*)(eaddr); paddr++) { \
    if (TM_SUCCESS != (cstr_ret = ato_check_set_tag(paddr, required_tag, new_tag))) { \
      break; \
    } \
  } \
  cstr_ret; })

#define ato_check_set_tag_zero_range(saddr, eaddr, required_tag, new_tag) ({ \
  tm_errcode_t cstr_ret = TM_SUCCESS; \
  for (uword_t* paddr = (uword_t*)(saddr); paddr < (uword_t*)(eaddr); paddr++) { \
    if (TM_SUCCESS != (cstr_ret = ato_check_set_tag_zero(paddr, required_tag, new_tag))) { \
      break; \
    } \
  } \
  cstr_ret; })

#else

typedef enum tag_type {
  T_NORMAL = 0,
  T_CALLABLE = 1,
  T_UTRUSTED = 2,
  T_MTRUSTED = 3
} tag_t;

tag_t load_tag(const void *addr);
void store_tag(const void *addr, tag_t tag);
tm_errcode_t check_tag_word(const void* addr, tag_t tag);

tm_errcode_t ato_check_set_tag_data(void* addr, tag_t required_tag, tag_t new_tag, uword_t data);
tm_errcode_t ato_check_set_tag(const void* addr, tag_t required_tag, tag_t new_tag);
tm_errcode_t ato_check_set_tag_range(void* saddr, void* eaddr, tag_t required_tag, tag_t new_tag);


#endif

#endif
