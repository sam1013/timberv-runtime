// See LICENSE for license details.

#include "trustmon.h"
#include "enclave.h"
#include "tag.h"
#include "pmp.h"
#include "track.h"
#include "measure.h"
#include "libc.h"
#include "hmac_sha2.h"
#include "mutex.h"

#define IS_ALIGNED(addr, bytes) !((uintptr_t)(addr) % (bytes))

/*
 * When using nested locking, always obey this locking sequence:
 * 1. global_ttcb_lock
 * 2. global_track_lock
 * 3. ttcb_check_lock
 * 4. global_mpu_lock
 * Unlocking has to be done in exact reverse order!
 */

/* internal functions */

static tm_errcode_t check_range_alignment(range_t* range) {
  if (((uword_t)range->start % sizeof(uword_t)) ||
      ((uword_t)range->end % sizeof(uword_t))) {
    return TM_INVALID;
  }
  return TM_SUCCESS;
}

static tm_errcode_t check_alignment(void* ptr) {
  if ((uword_t)ptr % sizeof(uword_t)) {
    return TM_INVALID;
  }
  return TM_SUCCESS;
}

/* ENCLS calls */

tm_errcode_t encls_create_enclave(ttcb_t* ttcb) {
  tm_errcode_t ret;
  ttcb_printf("encls_create_enclave entered\n");

  if (!ttcb) {
    return TM_INVALID;
  }

  if (TM_SUCCESS != (ret = global_ttcb_lock())) {
    ttcb_printf("encls_create_enclave: global_ttcb_lock failed\n");
    return ret;
  }

  if (TM_SUCCESS != (ret = global_track_lock())) {
    ttcb_printf("encls_create_enclave: global_track_lock failed\n");
    goto cleanup1;
  }

  /* check alignment */
  uintptr_t addr = (uintptr_t)ttcb;
  if (addr % sizeof(uword_t)) {
    ttcb_printf("encls_create_enclave: wrong ttcb alignment\n");
    ret = TM_INVALID;
    goto cleanup2;
  }

  /* initialize all but TTCB header */
  uword_t* ptr = (uword_t*)addr;
  uword_t* ptrend = (uword_t*)(addr + sizeof(ttcb_t));
  for (ptr++; ptr < ptrend; ptr++) {
    if (TM_SUCCESS != (ret = ato_check_set_tag_data(ptr, T_NORMAL, T_STRUSTED, 0))) {
      ret = TM_FATAL; /* inconsistent ttcb, non-recoverable state */
      ttcb_printf("encls_create_enclave: ato_check_set_tag_data failed\n");
      goto cleanup2;
    }
  }

  /* Noone but us can use newly created ttcb */
  ttcb_force_lock(ttcb);

  /* set magic header */
  if (TM_SUCCESS != (ret = ato_check_set_tag_data(&ttcb->magic, T_NORMAL, T_CALLABLE, TTCB_HEADER))) {
    ttcb_printf("encls_create_enclave: ato_check_set_tag_data on magic failed\n");
    ret = TM_FATAL; /* inconsistent ttcb, non-recoverable state */
    goto cleanup3;
  }

  if (TM_SUCCESS != (ret = track_ttcb(ttcb))) {
    ttcb_printf("encls_create_enclave: track_ttcb failed\n");
    ttcb_invalidate(ttcb); /* inconsistent tracking */
    goto cleanup3;
  }

#if TM_MEASURE == 1
  if (TM_SUCCESS != (ret = measure_init(ttcb))) {
    ttcb_printf("encls_create_enclave: measure_init failed\n");
    ttcb_invalidate(ttcb); /* inconsistent measurement */
    goto cleanup3;
  }
#endif
  ret = TM_SUCCESS;

cleanup3:
  ttcb_unlock(ttcb);
cleanup2:
  global_track_unlock();
cleanup1:
  global_ttcb_unlock();

  if (TM_SUCCESS != (ret = global_track_lock())) {
    ttcb_printf("encls_create_enclave2: global_track_lock failed\n");
    while(1);
  }
  global_track_unlock();
  return ret;
}

tm_errcode_t encls_add_enclave_region(ttcb_t* ttcb, void* start, void* end, uword_t flags, region_id_t idx) {
  tm_errcode_t ret;
  region_t region;
  ttcb_printf("encls_add_enclave_region entered\n");

  if (!ttcb) {
    return TM_INVALID;
  }

  if (TM_SUCCESS != (ret = global_track_lock())) {
    ttcb_printf("encls_add_enclave_region: global_track_lock failed\n");
    return ret;
  }

  if (TM_SUCCESS != (ret = ttcb_check_lock(ttcb))) {
    ttcb_printf("encls_add_enclave_region: ttcb_check_lock failed\n");
    goto cleanup1;
  }

  if (!ttcb_is_created(ttcb)) {
    ttcb_printf("encls_add_enclave_region: ttcb_is_created failed\n");
    ret = TM_INVALID;
    goto cleanup2;
  }

  region.range.start = start;
  region.range.end = end;
  region.flags = flags & PMP_ACCESS_UMASK;

  if (flags != region.flags) {
    ttcb_printf("encls_add_enclave_region: invalid region flags\n");
    ret = TM_INVALID;
    goto cleanup2;
  }

  if (TM_SUCCESS != (ret = check_range_alignment(&region.range))) {
    ttcb_printf("encls_add_enclave_region: check_range_alignment failed\n");
    goto cleanup2;
  }

  if (TM_SUCCESS != (ret = check_region_overlap(&region))) {
    ttcb_printf("encls_add_enclave_region: check_region_overlap failed\n");
    goto cleanup2;
  }

  if (TM_SUCCESS != (ret = ttcb_add_region_idx(ttcb, &region, idx))) {
    ttcb_printf("encls_add_enclave_region: ttcb_add_region failed\n");
    goto cleanup2;
  }

  if (TM_SUCCESS != (ret = track_region(&region))) {
    ttcb_printf("encls_add_enclave_region: track_region failed\n");
    ttcb_invalidate(ttcb); /* inconsistent tracking */
    goto cleanup2;
  }

#if TM_MEASURE == 1
  if (TM_SUCCESS != (ret = measure_region(ttcb, &region))) {
    ttcb_printf("encls_add_enclave_region: measure_region failed\n");
    ttcb_invalidate(ttcb); /* inconsistent measurement */
    goto cleanup2;
  }
#endif

  ret = TM_SUCCESS;

cleanup2:
  ttcb_unlock(ttcb);
cleanup1:
  global_track_unlock();
  return ret;
}

tm_errcode_t encls_add_data(ttcb_t* ttcb, void* start, void* end) {
  tm_errcode_t ret;
  range_t range;
  ttcb_printf("encls_add_data entered\n");

  if (!ttcb) {
    return TM_INVALID;
  }

  if (TM_SUCCESS != (ret = ttcb_check_lock(ttcb))) {
    ttcb_printf("encls_add_data: ttcb_check_lock failed\n");
    return ret;
  }

  if (!ttcb_is_created(ttcb)) {
    ttcb_printf("encls_add_data: ttcb_is_created failed\n");
    ret = TM_INVALID;
    goto cleanup;
  }

  range.start = start;
  range.end = end;
  if (TM_SUCCESS != (ret = check_range_alignment(&range))) {
    ttcb_printf("encls_add_data: check_range_alignment failed\n");
    goto cleanup;
  }

  if (TM_SUCCESS != (ret = check_is_range_within_ttcb(ttcb, &range))) {
    ttcb_printf("encls_add_data: check_is_range_within_ttcb failed\n");
    goto cleanup;
  }

  /* tag data memory */
  if (TM_SUCCESS != (ret = ato_check_set_tag_range(range.start, range.end, T_NORMAL, T_UTRUSTED))) {
    ttcb_printf("encls_add_data: tag_range failed\n");
    ttcb_invalidate(ttcb); /* unable to tag memory */
  }

#if TM_MEASURE == 1
  if (TM_SUCCESS != (ret = measure_data(ttcb, &range))) {
    ttcb_printf("encls_add_data: measure_data failed\n");
    ttcb_invalidate(ttcb); /* inconsistent measurement */
    goto cleanup;
  }
#endif
  ret = TM_SUCCESS;

cleanup:
  ttcb_unlock(ttcb);
  return ret;
}

tm_errcode_t encls_add_entries(ttcb_t* ttcb, void* entries[]) {
  tm_errcode_t ret;
  ttcb_printf("encls_add_entries entered\n");

  if (!ttcb || !entries) {
    return TM_INVALID;
  }

  if (TM_SUCCESS != (ret = ttcb_check_lock(ttcb))) {
    ttcb_printf("encls_add_entries: ttcb_check_lock failed\n");
    return ret;
  }

  void *entry;
  size_t i = 0;
  do {
    entry = (void*)DEREF_PTR_READ(&entries[i++], T_NORMAL);
    if (NULL == entry) {
      break;
    }

    if (TM_SUCCESS != (ret = check_alignment(entry))) {
      ttcb_printf("encls_add_entries: check_alignment failed\n");
      goto cleanup;
    }

    if (TM_SUCCESS != (ret = check_is_within_ttcb(ttcb, entry))) {
      ttcb_printf("encls_add_entries: check_is_within_ttcb failed\n");
      goto cleanup;
    }

    if (TTCB_HEADER == DEREF_PTR_READ(entry, T_UTRUSTED)) {
      ttcb_printf("encls_add_entries: entry contains forbidden TTCB_HEADER\n");
      goto cleanup;
    }

    /* tag entry */
    /* Entry is T_UTRUSTED since it was added via encls_add_data */
    if (TM_SUCCESS != (ret = ato_check_set_tag(entry, T_UTRUSTED, T_CALLABLE))) {
      ttcb_printf("encls_add_entries: ato_check_set_tag failed\n");
      goto cleanup;
    }

#if TM_MEASURE == 1
    if (TM_SUCCESS != (ret = measure_entry(ttcb, entry))) {
      ttcb_printf("encls_add_entries: measure_entry failed\n");
      ttcb_invalidate(ttcb); /* inconsistent measurement */
      goto cleanup;
    }
#endif
  } while (NULL != entry);

  ret = TM_SUCCESS;

cleanup:
  ttcb_unlock(ttcb);
  return ret;
}

tm_errcode_t encls_init_enclave(ttcb_t* ttcb) {
  tm_errcode_t ret;
  ttcb_printf("encls_init_enclave entered\n");

  if (!ttcb) {
    return TM_INVALID;
  }

  if (TM_SUCCESS != (ret = ttcb_check_lock(ttcb))) {
    ttcb_printf("encls_init_enclave: ttcb_check_lock failed\n");
    return ret;
  }

  if (!ttcb_is_created(ttcb)) {
    ttcb_printf("encls_init_enclave: ttcb_is_created failed\n");
    ret = TM_INVALID;
    goto cleanup;
  }

#if TM_MEASURE == 1
  if (TM_SUCCESS != (ret = measure_finalize(ttcb))) {
    ttcb_printf("encls_init_enclave: measure_finalize failed\n");
    ttcb_invalidate(ttcb); /* inconsistent measurement */
    goto cleanup;
  }
#endif
  ttcb_mark_ready(ttcb);
  ttcb_print(ttcb);
  ret = TM_SUCCESS;

cleanup:
  ttcb_unlock(ttcb);
  return ret;
}

tm_errcode_t encls_load_enclave(ttcb_t* ttcb) {
  tm_errcode_t ret;
  ttcb_printf("encls_load_enclave entered\n");

  if (!ttcb) {
    return TM_INVALID;
  }

  if (TM_SUCCESS != (ret = ttcb_check_lock(ttcb))) {
    ttcb_printf("encls_load_enclave: ttcb_check_lock failed with %d\n", ret);
    return ret;
  }

  if (!ttcb_is_ready(ttcb)) {
    ttcb_printf("encls_load_enclave: ttcb_is_ready failed!\n");
    ret = TM_INVALID;
    goto cleanup;
  }
  if (TM_SUCCESS != (ret = mpu_load_ttcb_locked(ttcb))) {
    ttcb_printf("encls_load_enclave: ttcb_load_mpu failed!\n");
    goto cleanup;
  }

  /* ttcb remains locked while loaded in the mpu */
  return TM_SUCCESS;

cleanup:
  ttcb_unlock(ttcb);
  return ret;
}

tm_errcode_t encls_destroy_enclave(ttcb_t* ttcb) {
  tm_errcode_t ret = TM_SUCCESS;
  ttcb_printf("encls_destroy_enclave entered\n");

  if (!ttcb) {
    return TM_INVALID;
  }

  if (TM_SUCCESS != (ret = global_track_lock())) {
    ttcb_printf("encls_destroy_enclave: global_track_lock failed\n");
    return ret;
  }

  if (TM_SUCCESS != (ret = ttcb_check_lock(ttcb))) {
    ttcb_printf("encls_destroy_enclave: ttcb_check_lock failed\n");
    return ret;
  }

  ttcb_invalidate(ttcb);    /* prevent enclave from reloading */

  for (region_id_t i = 0; i < MAX_TTCB_REGIONS; i++) {
    region_t* region = ttcb_get_region(ttcb, i);
    if (NULL == region) {
      continue;
    }
    /* SHM is cleared by owner */
    if (!(region->flags & PMP_SOFT_SHM)) {
      if (TM_SUCCESS != ttcb_clear_region(ttcb, i)) {
        ttcb_printf("encls_destroy_enclave: ttcb_clear_region failed\n");
        ret = TM_FATAL; /* non-recoverable state */
        goto error;
      }
    }
    if (TM_SUCCESS != ttcb_remove_region(ttcb, i)) {
      ttcb_printf("encls_destroy_enclave: ttcb_remove_region failed\n");
      ret = TM_FATAL; /* non-recoverable state */
      goto error;
    }
    if (TM_SUCCESS != untrack_region(region)) {
      ttcb_printf("encls_destroy_enclave: untrack_region failed\n");
      ret = TM_FATAL; /* non-recoverable state */
      goto error;
    }
  }

  if (TM_SUCCESS != untrack_ttcb(ttcb)) {
    ttcb_printf("encls_destroy_enclave: untrack_ttcb failed\n");
    ret = TM_FATAL;
    goto error;
  }

  /* destroy magic header */
  if (TM_SUCCESS != ato_check_set_tag_data(ttcb, T_CALLABLE, T_NORMAL, 0)) {
    ttcb_printf("encls_destroy_enclave: ato_check_set_tag_data(ttcb header) failed\n");
    ret = TM_FATAL; /* non-recoverable state */
    goto cleanup;
  }

  /* clear all but TTCB header */
  uintptr_t addr = (uintptr_t)ttcb;
  uword_t* ptr = (uword_t*)ttcb;
  uword_t* ptrend = (uword_t*)(addr + sizeof(ttcb_t));
  for (ptr++; ptr < ptrend; ptr++) {
    if (TM_SUCCESS != (ret = ato_check_set_tag_data(ptr, T_STRUSTED, T_NORMAL, 0))) {
      ttcb_printf("encls_destroy_enclave: ato_check_set_tag_data(ttcb data) failed\n");
      ret = TM_FATAL; /* non-recoverable state */
      goto cleanup;
    }
  }
  ret = TM_SUCCESS;
  goto cleanup;

error:
  ttcb_unlock(ttcb);
cleanup:
  global_track_unlock();
  return ret;
}

/* ENCLU calls */

static mutex_t mutex_crypto = 0;

/* Used for accessing globally shared crypto contexts such as
 * hmac_ctx
 */
tm_errcode_t global_crypto_lock() {
  if (MUTEX_TRY_LOCK(mutex_crypto)) {
    return TM_LOCKED;
  }
  return TM_SUCCESS;
}

void global_crypto_unlock() {
  MUTEX_UNLOCK(mutex_crypto);
}

static hmac_sha256_ctx hmac_ctx;
static unsigned char tmp_buf[TM_TMP_SIZE];
static unsigned char platform_key[TM_KEY_SIZE] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

void init_crypto() {
  hmac_sha256_init(&hmac_ctx, platform_key, TM_KEY_SIZE);
}

void tagged_copy_from_ut(void* dst, void* src, size_t len) {
  unsigned char* pdst = dst;
  unsigned char* psrc = src;
  while (len--) {
    *pdst = DEREF_BYTE_PTR_READ(psrc, T_UTRUSTED);
    psrc++;
    pdst++;
  }
}

void tagged_copy_to_ut(void* dst, void* src, size_t len) {
  unsigned char* pdst = dst;
  unsigned char* psrc = src;
  while (len--) {
    DEREF_BYTE_PTR_WRITE(pdst, T_UTRUSTED, *psrc);
    psrc++;
    pdst++;
  }
}

tm_errcode_t enclu_getkey(ttcb_t* ttcb, unsigned char* msg, unsigned int msg_len, unsigned char* key) {
  ttcb_printf("enclu_getkey called\n");

  if (!ttcb || !msg || !key) {
    return TM_INVALID;
  }

  tm_errcode_t ret;
  range_t range;
  range.start = msg;
  range.end = msg + msg_len;

  if (TM_SUCCESS != (ret = check_is_range_within_ttcb(ttcb, &range))) {
    ttcb_printf("enclu_getkey: check_is_range_within_ttcb failed for msg\n");
    return ret;
  }

  range.start = key;
  range.end = key + TM_KEY_SIZE;
  if (TM_SUCCESS != (ret = check_is_range_within_ttcb(ttcb, &range))) {
    ttcb_printf("enclu_getkey: check_is_range_within_ttcb failed for key\n");
    return ret;
  }

  if (TM_SUCCESS != (ret = global_crypto_lock())) {
    ttcb_printf("enclu_getkey: global_crypto_lock failed\n");
    return ret;
  }
#if TM_MEASURE == 1
  hmac_sha256_reinit(&hmac_ctx);
  hmac_sha256_update(&hmac_ctx, ttcb->identity, EID_SIZE);
  while (msg_len > 0) {
    size_t tmp_len = msg_len > TM_TMP_SIZE ? TM_TMP_SIZE : msg_len;
    tagged_copy_from_ut(tmp_buf, msg, tmp_len);
    hmac_sha256_update(&hmac_ctx, tmp_buf, tmp_len);
    msg += tmp_len;
    msg_len -= tmp_len;
  }
  hmac_sha256_final(&hmac_ctx, tmp_buf, TM_KEY_SIZE);
  tagged_copy_to_ut(key, tmp_buf, TM_KEY_SIZE);
#endif
  global_crypto_unlock();

  return TM_SUCCESS;
}

tm_errcode_t enclu_shm_offer(ttcb_t* ttcb, unsigned char* target_eid, region_t* region) {
  enclu_printf("enclu_offer called\n");

  if (!ttcb || !target_eid || !region) {
    return TM_INVALID;
  }

  tm_errcode_t ret = TM_SUCCESS;
  range_t range;
  range.start = target_eid;
  range.end = target_eid + EID_SIZE;
  if (TM_SUCCESS != (ret = check_is_range_within_ttcb(ttcb, &range))) {
    ttcb_printf("enclu_shm_offer: check_is_range_within_ttcb failed for target_eid\n");
    return ret;
  }

  range.start = region;
  range.end = region+1;
  if (TM_SUCCESS != (ret = check_is_range_within_ttcb(ttcb, &range))) {
    ttcb_printf("enclu_shm_offer: check_is_range_within_ttcb failed for region\n");
    return ret;
  }

  region_t stregion;
  tagged_copy_from_ut(&stregion, region, sizeof(region_t));

  if (stregion.flags != (stregion.flags & PMP_ACCESS_UMASK)) {
    ttcb_printf("enclu_shm_offer: invalid flags\n");
    return TM_INVALID;
  }

  if (TM_SUCCESS != (ret = check_is_region_within_ttcb(ttcb, &stregion))) {
    ttcb_printf("enclu_shm_offer: check_is_range_within_ttcb failed for region->range\n");
    return ret;
  }

  /* TODO: is global lock necessary? */
  if (TM_SUCCESS != (ret = global_track_lock())) {
    ttcb_printf("enclu_shm_close: global_track_lock failed\n");
    return ret;
  }

  tagged_copy_from_ut(ttcb->shm_offer_target_eid, target_eid, EID_SIZE);
  ttcb->shm_offer_region = stregion;
  global_track_unlock();

  ttcb_print(ttcb);
  return TM_SUCCESS;
}

tm_errcode_t enclu_shm_accept(ttcb_t* ttcb, unsigned char* owner_eid, void** start, void** end) {
  enclu_printf("enclu_shm_accept entered\n");

  if (!ttcb || !owner_eid || !start || !end) {
    return TM_INVALID;
  }

  tm_errcode_t ret = TM_SUCCESS;
  range_t range;
  range.start = owner_eid;
  range.end = owner_eid + EID_SIZE;
  if (TM_SUCCESS != (ret = check_is_range_within_ttcb(ttcb, &range))) {
    ttcb_printf("enclu_shm_accept: check_is_range_within_ttcb failed for owner_eid\n");
    return ret;
  }
  tagged_copy_from_ut(tmp_buf, owner_eid, EID_SIZE);

  if (TM_SUCCESS != (ret = check_is_within_ttcb(ttcb, start))) {
    ttcb_printf("enclu_shm_accept: check_is_within_ttcb failed for start\n");
    return ret;
  }

  if (TM_SUCCESS != (ret = check_is_within_ttcb(ttcb, end))) {
    ttcb_printf("enclu_shm_accept: check_is_within_ttcb failed for end\n");
    return ret;
  }

  if (TM_SUCCESS != (ret = global_track_lock())) {
    ttcb_printf("enclu_shm_close: global_track_lock failed\n");
    return ret;
  }

  region_t region;
  if (TM_SUCCESS != (ret = check_shm_offer(ttcb, tmp_buf, &region))) {
    ttcb_printf("enclu_shm_accept: check_shm_offer failed\n");
    goto cleanup;
  }

  DEREF_PTR_WRITE(start, T_UTRUSTED, region.range.start);
  DEREF_PTR_WRITE(end, T_UTRUSTED, region.range.end);
  ttcb_print(ttcb);
cleanup:
  global_track_unlock();
  return ret;
}

tm_errcode_t enclu_shm_close(ttcb_t* ttcb, void* start, void* end) {
  ttcb_printf("enclu_shm_close entered\n");

  if (!ttcb) {
    return TM_INVALID;
  }

  tm_errcode_t ret = TM_SUCCESS;
  if (TM_SUCCESS != (ret = global_track_lock())) {
    ttcb_printf("enclu_shm_close: global_track_lock failed\n");
    return ret;
  }

  for (region_id_t i = 0; i < MAX_TTCB_REGIONS; i++) {
    region_t* region = ttcb_get_region(ttcb, i);
    if (NULL == region) {
      continue;
    }
    if (region->flags & PMP_SOFT_SHM) {
      if (region->range.start == start && region->range.end == end) {
        /* We found the region */
        if (TM_SUCCESS != ttcb_remove_region(ttcb, i)) {
          ttcb_printf("enclu_shm_close: ttcb_remove_region failed\n");
          ret = TM_FATAL; /* non-recoverable state */
          break;
        }
        if (TM_SUCCESS != untrack_region(region)) {
          ttcb_printf("enclu_shm_close: untrack_region failed\n");
          ret = TM_FATAL; /* non-recoverable state */
          break;
        }
        break;
      }
    }
  }

  /* TODO: yield process in order to flush MPU */
  global_track_unlock();
  return ret;
}

#if TM_BENCH == 1
extern tm_errcode_t enclu_bench();
#endif

tm_errcode_t enclu_dispatcher(long number, long arg1, long arg2, long arg3) {
  enclu_printf("enclu_dispatcher %x %x %x %x\n", number, arg1, arg2, arg3);
  ttcb_t* ttcb = (ttcb_t*)read_csr(sttcb);
  tm_errcode_t ret;
  switch (number) {
    case ENCLU_GETKEY:
      return enclu_getkey(ttcb, (unsigned char*)arg1, arg2, (unsigned char*)arg3);
    case ENCLU_SHMOFFER:
      return enclu_shm_offer(ttcb, (unsigned char*)arg1, (region_t*)arg2);
    case ENCLU_SHMACCEPT:
      return enclu_shm_accept(ttcb, (unsigned char*)arg1, (void*)arg2, (void*)arg3);
    case ENCLU_SHMCLOSE:
      return enclu_shm_close(ttcb, (void*)arg1, (void*)arg2);
#if TM_BENCH == 1
    case ENCLU_BENCH:
      return enclu_bench();
#endif
    default:
      return TM_INVALID;
  }
  return TM_SUCCESS;
}

void st_fail(uword_t scause, uword_t sepc, uword_t stval, uword_t sstatus, uword_t ststatus, uword_t sttcb) {
  tm_printf("ST FATAL\n");
  tm_printf("scause   = %08x\n", scause);
  tm_printf("sepc     = %08x\n", sepc);
  tm_printf("stval    = %08x\n", stval);
  tm_printf("sstatus  = %08x\n", sstatus);
  tm_printf("ststatus = %08x\n", ststatus);
  tm_printf("sttcb    = %08x\n", sttcb);
  while(1);
}
