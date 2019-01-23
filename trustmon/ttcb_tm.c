// See LICENSE for license details.

#include "trustmon.h"
#include <stddef.h>
#include "ttcb.h"
#include "pmp.h"
#include "tag.h"
#include "mutex.h"
#include "libc.h"

/* Lock for all tag changes from/to tag.M and tag.C as well as for
 * locking TTCB's */
tm_errcode_t global_ttcb_lock() {
  if (MUTEX_TRY_LOCK(mutex_ttcb)) {
    return TM_LOCKED;
  }
  return TM_SUCCESS;
}

void global_ttcb_unlock() {
  MUTEX_UNLOCK(mutex_ttcb);
}

void ttcb_print(ttcb_t* ttcb) {
#if TM_PRINTF == 1
  if (ttcb == NULL) {
    return;
  }
  tm_printf("TTCB @ %x\n", (uword_t)ttcb);
  tm_printf("status:\n");
  tm_printf("  state=%d\n", ttcb->status.state);
  tm_printf("  interrupted=%d\n", ttcb->status.interrupted);
  tm_printf("  locked=%d\n", ttcb->status.locked);
  for (region_id_t i = 0; i < MAX_TTCB_REGIONS; i++) {
    region_t* region = ttcb_get_region(ttcb, i);
    if (region == NULL) {
      break;
    }
    tm_printf("region[%d]: 0x%08x - 0x%08x [0x%08x]\n", i,
      region->range.start,
      region->range.end,
      region->flags);
  }
  tm_printf("EID: ");
  for (size_t i = 0; i < sizeof(ttcb->identity); i++) {
    tm_printf("0x%02X,", ttcb->identity[i]);
  }
  tm_printf("\n");
  tm_printf("SHM target EID: ");
  for (size_t i = 0; i < sizeof(ttcb->shm_offer_target_eid); i++) {
    tm_printf("0x%02X,", ttcb->shm_offer_target_eid[i]);
  }
  tm_printf("\n");
  tm_printf("SHM offer region: 0x%08x - 0x%08x [0x%08x]\n",
      ttcb->shm_offer_region.range.start,
      ttcb->shm_offer_region.range.end,
      ttcb->shm_offer_region.flags);
#endif
}

/*
 * Checks for a valid ttcb and locks it.
 * In case ttcb is loaded in the mpu, it is automatically unloaded.
 */
tm_errcode_t ttcb_check_lock(ttcb_t* ttcb) {
  tm_errcode_t ret;
  if (TM_SUCCESS != (ret = global_ttcb_lock())) {
    tm_printf("ttcb_check_lock: global_ttcb_lock failed\n");
    return ret;
  }

  uintptr_t addr = (uintptr_t)ttcb;

  /* check alignment */
  if (addr % sizeof(uintptr_t)) {
    tm_printf("ttcb_check_lock: wrong alignment\n");
    ret = TM_INVALID;
    goto cleanup;
  }

  /* check TTCB header */
  if (TTCB_HEADER != DEREF_PTR_READ(&ttcb->magic, T_CALLABLE)) {
    tm_printf("ttcb_check_lock: wrong TTCB header\n");
    ret = TM_INVALID;
    goto cleanup;
  }

  if (ttcb->status.locked) {
    /* Check if ttcb is currently loaded in the mpu */
    /* In that case, unload ttcb, which is already locked */
    if (TM_SUCCESS != (ret = mpu_unload_ttcb_locked(ttcb))) {
      if (TM_LOCKED == ret) {
        tm_printf("mpu_unload_ttcb_locked: locked\n");
      } else {
        tm_printf("ttcb_check_lock: locked\n");
      }
      ret = TM_LOCKED;
      goto cleanup;
    }
  } else {
    ttcb_force_lock(ttcb);
  }
  ret = TM_SUCCESS;
cleanup:
  global_ttcb_unlock();
  return ret;
}

tm_errcode_t check_is_within_ttcb(ttcb_t* ttcb, void* ptr) {
  for (size_t i = 0; i < MAX_TTCB_REGIONS; i++) {
    if (ttcb->region[i].flags & PMP_ACCESS_UT) {
      if (((uintptr_t)ttcb->region[i].range.start <= (uintptr_t)ptr) &&
          ((uintptr_t)ttcb->region[i].range.end   >= (uintptr_t)ptr + sizeof(uword_t))) {
        return TM_SUCCESS;
      }
    }
  }
  return TM_INVALID;
}

/**
 * region must be a subset of at least one ttcb region, both
 * in the memory range and in the flags
 */
tm_errcode_t check_is_region_within_ttcb(ttcb_t* ttcb, region_t* region) {
  for (size_t i = 0; i < MAX_TTCB_REGIONS; i++) {
    if ((ttcb->region[i].flags & PMP_ACCESS_UT) &&
        /* region.flags must be subset of ttcb->region */
        (ttcb->region[i].flags == (ttcb->region[i].flags | region->flags))) {
      if (((uintptr_t)ttcb->region[i].range.start <= (uintptr_t)region->range.start) &&
          ((uintptr_t)ttcb->region[i].range.end   >=  (uintptr_t)region->range.end)) {
        return TM_SUCCESS;
      }
    }
  }
  return TM_INVALID;
}

tm_errcode_t check_is_range_within_ttcb(ttcb_t* ttcb, range_t* range) {
  for (size_t i = 0; i < MAX_TTCB_REGIONS; i++) {
    if (ttcb->region[i].flags & PMP_ACCESS_UT) {
      // check whether x1:x2 includes y1:y2
      // check x1 <= y1 and x2 >= y2
      if (((uintptr_t)ttcb->region[i].range.start <= (uintptr_t)range->start) &&
          ((uintptr_t)ttcb->region[i].range.end   >=  (uintptr_t)range->end)) {
        return TM_SUCCESS;
      }
    }
  }
  return TM_INVALID;
}

tm_errcode_t check_is_range_overlapping_ttcb(ttcb_t* ttcb, range_t* range) {
  for (size_t i = 0; i < MAX_TTCB_REGIONS; i++) {
    if (ttcb->region[i].flags & PMP_ACCESS_UT) {
      // check whether x1:x2 intersects y1:y2
      // check x1 <= y2 and y1 <= x2
      if (((uintptr_t)ttcb->region[i].range.start <= (uintptr_t)range->end) &&
          ((uintptr_t)ttcb->region[i].range.end   >=  (uintptr_t)range->start)) {
        return TM_SUCCESS;
      }
    }
  }
  return TM_INVALID;
}

region_t* ttcb_get_region(ttcb_t* ttcb, region_id_t idx) {
  if (idx >= MAX_TTCB_REGIONS || !ttcb->region[idx].flags) {
    return NULL;
  }
  return &ttcb->region[idx];
}

/*
 * Caller must hold global_track_lock!
 */
tm_errcode_t ttcb_add_region_idx(ttcb_t* ttcb, region_t* region, region_id_t idx) {
  if (idx >= MAX_TTCB_REGIONS || ttcb->region[idx].flags) {
    return TM_INVALID;
  }
  tm_printf("ttcb_add_region idx %d\n", idx);
  ttcb->region[idx] = *region;
  return TM_SUCCESS;
}

tm_errcode_t ttcb_add_region(ttcb_t* ttcb, region_t* region) {
  for (size_t i = 0; i < MAX_TTCB_REGIONS; i++) {
    if (TM_SUCCESS == ttcb_add_region_idx(ttcb, region, i)) {
      return TM_SUCCESS;
    }
  }
  return TM_INVALID;
}

/*
 * Clear all region's tag.t memory and tag.C entry points
 */
tm_errcode_t ttcb_clear_region(ttcb_t* ttcb, region_id_t idx) {
  tm_errcode_t ret;
  if (idx >= MAX_TTCB_REGIONS || !ttcb->region[idx].flags) {
    return TM_INVALID;
  }

  region_t region = ttcb->region[idx];
  uword_t* ptr = region.range.start;
  for (; ptr < (uword_t*)region.range.end; ptr++) {
    if (load_test_tag(ptr, T_UTRUSTED) == 0) {
      if (TM_SUCCESS != (ret = ato_check_set_tag_data(ptr, T_UTRUSTED, T_NORMAL, 0))) {
        goto cleanup;
      }
    } else if (load_test_tag(ptr, T_CALLABLE) == 0) {
      if (TM_SUCCESS != (ret = ato_check_set_tag_data(ptr, T_CALLABLE, T_NORMAL, 0))) {
        goto cleanup;
      }
    } else if (load_test_tag(ptr, T_STRUSTED) == 0) {
      /* This can happen when destroying an enclave interrupted during ENCLU call with stack interleaving. */
      /* However, if ENCLS call uses stack interleaving, we would destroy our own stack */
      //~ if (TM_SUCCESS != (ret = ato_check_set_tag_data(ptr, T_STRUSTED, T_NORMAL, 0))) {
        //~ goto cleanup;
      //~ }
    }
  }
  ret = TM_SUCCESS;
cleanup:
  return ret;
}

/*
 * Invalidate a region
 *
 * Caller must hold global_track_lock!
 */
tm_errcode_t ttcb_remove_region(ttcb_t* ttcb, region_id_t idx) {
  if (idx >= MAX_TTCB_REGIONS || !ttcb->region[idx].flags) {
    return TM_INVALID;
  }
  ttcb->region[idx].flags = 0;
  return TM_SUCCESS;
}
