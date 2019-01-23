// See LICENSE for license details.

#include "trustmon.h"
#include "ttcb.h"
#include "pmp.h"
#include "encoding.h"
#include "mutex.h"
#include "libc.h"

static mutex_t mutex_mpu = 0;

/* Lock MPU / MTSTATUS / MTTCB csr */
tm_errcode_t global_mpu_lock() {
  if (MUTEX_TRY_LOCK(mutex_mpu)) {
    return TM_LOCKED;
  }
  return TM_SUCCESS;
}

void global_mpu_unlock() {
  MUTEX_UNLOCK(mutex_mpu);
}

#define unload_mpu_range(x) clear_csr(spmpflags##x, PMP_ACCESS_A)

/*
 * Unload currently loaded enclave and store interrupted state into TTCB.
 * Returns the locked ttcb, or NULL.
 * This function requires global_mpu_lock
 */
__attribute__((always_inline))
static ttcb_t* mpu_unload_internal() {

  uword_t status = read_csr(ststatus);
  clear_csr(ststatus, TSTATUS_UE);
  clear_csr(ststatus, TSTATUS_UI);

  asm_unack_mpu();
  
  ttcb_t* ttcb = (ttcb_t*)read_csr(sttcb);
  if (NULL != ttcb) {
    ttcb->status.interrupted = !!(status & TSTATUS_UI);
  }
  write_csr(sttcb, 0);
  return ttcb;
}

/*
 * If ttcb is currently loaded, unloads ttcb, keeps it locked and returns TM_SUCCESS.
 * Otherwise, returns TM_NOT_FOUND.
 */
tm_errcode_t mpu_unload_ttcb_locked(ttcb_t* ttcb) {
  tm_errcode_t ret;
  if (TM_SUCCESS != (ret = global_mpu_lock())) {
    return ret;
  }
  ttcb_t* current_ttcb = (ttcb_t*)read_csr(sttcb);
  if (current_ttcb == ttcb) {
    mpu_unload_internal();
    ret = TM_SUCCESS;
  } else {
    ret = TM_NOT_FOUND;
  }
  global_mpu_unlock();
  return ret;
}

/* 
 * Loads ttcb into mpu, unloading previously loaded enclave
 */
tm_errcode_t mpu_load_ttcb_locked(ttcb_t* ttcb) {
  tm_errcode_t ret;
  if (TM_SUCCESS != (ret = global_mpu_lock())) {
    return ret;
  }

  /* Unload previously loaded enclave */
  ttcb_t* old_ttcb = mpu_unload_internal();
  if (old_ttcb) {
    ttcb_unlock(old_ttcb);
  }

  ret = asm_ack_mpu(ttcb->region);

  write_csr(sttcb, ttcb);

  /* sttcb.UE/UI are already cleared by mpu_unload_internal */
  if (ttcb_is_interrupted(ttcb)) {
    set_csr(ststatus, TSTATUS_UI);
  }
  if (ttcb_is_ready(ttcb)) {
    set_csr(ststatus, TSTATUS_UE);
  }
  global_mpu_unlock();
  return ret;
}
