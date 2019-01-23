// See LICENSE for license details.

#include "trustmon.h"
#include "track.h"
#include "mutex.h"
#include "libc.h"
#include "string.h"
#include "bench.h"

ttcb_t* ttcb_list_head = NULL;
static mutex_t mutex_track = 0;

/* Whenever scanning global tracking (across all ttcb's) one needs to
 * acquire global_track_lock.
 *
 * Whenever changing a range in the tracking, one needs to acquire
 * global_track_lock.
 *
 * Whenever checking local ttcb's ranges only (e.g. membership tests),
 * it suffices to lock ttcb only.
 */
tm_errcode_t global_track_lock() {
  if (MUTEX_TRY_LOCK(mutex_track)) {
    return TM_LOCKED;
  }
  return TM_SUCCESS;
}

void global_track_unlock() {
  MUTEX_UNLOCK(mutex_track);
}

/*
 * Caller must hold global_track_lock!
 */
/* This is implicitly done by adding region to ttcb that is already tracked */
tm_errcode_t track_region(region_t* region) {
  return TM_SUCCESS;
}

/*
 * Caller must hold global_track_lock!
 */
/* This is implicitly done by removing region to ttcb that is already tracked */
tm_errcode_t untrack_region(region_t* region) {
  return TM_SUCCESS;
}

/*
 * Caller must hold global_track_lock!
 */
tm_errcode_t track_ttcb(ttcb_t* ttcb) {
  if (ttcb_list_head == NULL) {
    ttcb_list_head = ttcb;
  } else {
    ttcb_t* current = ttcb_list_head;
    while (current->next) {
      current = current->next;
    }
    current->next = ttcb;
  }
  return TM_SUCCESS;
}

/*
 * Caller must hold global_track_lock!
 */
tm_errcode_t untrack_ttcb(ttcb_t* ttcb) {
  if (ttcb_list_head == ttcb) {
    ttcb_list_head = ttcb->next;
    return TM_SUCCESS;
  }
  ttcb_t* current = ttcb_list_head;
  while (NULL != current) {
    if (current->next == ttcb) {
      current->next = ttcb->next;
      return TM_SUCCESS;
    }
    current = current->next;
  }
  return TM_INVALID;
}

/*
 * Caller must hold global_track_lock!
 * return TM_INVALID if there's an overlap or TM_SUCCESS if there is no overlap
 */
tm_errcode_t check_region_overlap(region_t* region) {
  ttcb_t* current = ttcb_list_head;
  while (NULL != current) {
    if (TM_SUCCESS == check_is_range_overlapping_ttcb(current, &region->range)) {
      /* We detected an overlap */
      return TM_INVALID;
    }
    current = current->next;
  }
  return TM_SUCCESS;
}

int secure_memeq(void* a1, void* a2, size_t len) {
  unsigned char* pa1 = a1;
  unsigned char* pa2 = a2;
  int res = 0;
  for (size_t i = 0; i < len; i++) {
    res |= pa1[i] ^ pa2[i];
  }
#if TM_BENCH == 1
  /* For benchmarking, we match the first enclave as shm owner and the first enclave's offer as ours */
  return 1;
#endif
  return res == 0 ? 1 : 0;
}

/*
 * Caller must hold global_track_lock!
 */
tm_errcode_t check_shm_offer(ttcb_t* ttcb, unsigned char* owner_eid, region_t* region) {
  tm_errcode_t ret = TM_SUCCESS;
  ttcb_t* current = ttcb_list_head;
  while (NULL != current) {
    ttcb_printf("check_shm_offer: testing ttcb @ %x\n", current);
    if (secure_memeq(current->shm_offer_target_eid, ttcb->identity, EID_SIZE)) {
      ttcb_printf("check_shm_offer: found offered shm\n");
      if (secure_memeq(current->identity, owner_eid, EID_SIZE)) {
        ttcb_printf("check_shm_offer: found owner\n");
        if (TM_SUCCESS != (ret = ttcb_add_region(ttcb, &current->shm_offer_region))) {
          ttcb_printf("check_shm_offer: ttcb_add_region failed\n");
          return ret;
        }
        *region = current->shm_offer_region;
        return TM_SUCCESS;
      }
    }
    current = current->next;
  }
  return TM_NOT_FOUND;
}
