// See LICENSE for license details.

#include "trustmon.h"
#include "tag.h"
#include "enclave.h"
#include "encoding.h"
#include "pmp.h"
#include "libc.h"
#include "untrusted.h"

extern void core_main();
extern void tm_encls_bench();

/* ST functions that can be directly called by SN */
const void* callables[] = {
  &tm_encls_create_enclave,
  &tm_encls_add_enclave_region,
  &tm_encls_add_data,
  &tm_encls_add_entries,
  &tm_encls_init_enclave,
  &tm_encls_load_enclave,
  &tm_encls_resume_enclave,
  &tm_encls_destroy_enclave,
#if TM_BENCH == 1
  &tm_encls_bench,
#endif
#if TM_PRINTF == 1
  &tm_printf_exit,
#endif
  NULL,
};

/* Initialize ST trustmon
 * This is called from machine mode and does not use checked load/store
 * Do not use syscall / printf since this will drop priv. from M to S-mode!
 */
void tm_init()
{
  extern uintptr_t _trustmon_functions_start[];
  extern uintptr_t _trustmon_functions_end[];
  extern uintptr_t _trustmon_data_start[];
  extern uintptr_t _trustmon_data_end[];

  assert(ato_check_set_tag_range(_trustmon_functions_start, _trustmon_functions_end, T_NORMAL, T_STRUSTED) == TM_SUCCESS);
  assert(ato_check_set_tag_range(_trustmon_data_start,      _trustmon_data_end,      T_NORMAL, T_STRUSTED) == TM_SUCCESS);

  /* set tag on callables */
  for (size_t i = 0; callables[i]; i++) {
    assert(ato_check_set_tag(callables[i], T_STRUSTED, T_CALLABLE) == TM_SUCCESS);
  }

  /* set MPU */
  SET_PMP(6, _trustmon_functions_start, _trustmon_functions_end, PMP_ACCESS_ST | PMP_ACCESS_X);
  SET_PMP(7, _trustmon_data_start, _trustmon_data_end, PMP_ACCESS_ST | PMP_ACCESS_R | PMP_ACCESS_W);

  /* activate trust */
  set_csr(mtstatus, TSTATUS_EN);
  init_crypto();
}
