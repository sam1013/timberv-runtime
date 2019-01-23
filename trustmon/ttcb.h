// See LICENSE for license details.

#ifndef TTCB_H
#define TTCB_H

#include "trustmon.h"
#include "sha256.h"
#include <stdint.h>
#include "mutex.h"

#if __riscv_xlen == 64
#define TTCB_HEADER 0xa001a001a001a001ULL /* 4x assembler code "j ." */
#else
#define TTCB_HEADER 0xa001a001UL          /* 2x assembler code "j ." */
#endif

typedef struct __attribute__((packed)) {
  void* start;
  void* end; //exclusive
} range_t;

typedef struct __attribute__((packed)) {
  range_t range;
  uword_t flags;
} region_t;

typedef enum {
  created = 0,
  ready = 1,
  destroyed = 2, //corrupt
} enclave_state_t;

typedef struct __attribute__((packed)) {
  enclave_state_t state : 2;
  uint8_t interrupted : 1;
  uint8_t locked : 1;
  uint8_t unused : 4;
  uint8_t unused2[7];
} ttcb_status_t;

typedef unsigned char region_id_t;
#define MAX_TTCB_REGIONS 4

typedef struct ttcb_t ttcb_t;

#define EID_SIZE 32

struct __attribute__((packed)) ttcb_t {
  uword_t magic;                          /* 4b/8b */
  void* base;
  region_t region[MAX_TTCB_REGIONS];      /* 24*4b. Might only be modified when global_track_lock is hold! */
  region_t shm_offer_region;
  unsigned char shm_offer_target_eid[EID_SIZE];
  sha256_t hash;
  unsigned char identity[EID_SIZE];
  ttcb_status_t status;
  ttcb_t* next;                           /* Points to next ttcb. Might only be modified when global_track_lock is hold! */
}; /* Size must be multiple of uword_t */

#define INLINE __attribute__((always_inline)) static inline

static mutex_t mutex_ttcb = 0;

tm_errcode_t global_ttcb_lock();
void global_ttcb_unlock();

void ttcb_print(ttcb_t* ttcb);
tm_errcode_t ttcb_check_lock(ttcb_t* ttcb);
tm_errcode_t check_is_within_ttcb(ttcb_t* ttcb, void* ptr);
tm_errcode_t check_is_region_within_ttcb(ttcb_t* ttcb, region_t* region);
tm_errcode_t check_is_range_within_ttcb(ttcb_t* ttcb, range_t* range);
tm_errcode_t check_is_range_overlapping_ttcb(ttcb_t* ttcb, range_t* range);
region_t* ttcb_get_region(ttcb_t* ttcb, region_id_t idx);
tm_errcode_t ttcb_add_region_idx(ttcb_t* ttcb, region_t* region, region_id_t idx);
tm_errcode_t ttcb_add_region(ttcb_t* ttcb, region_t* region);
tm_errcode_t ttcb_clear_region(ttcb_t* ttcb, region_id_t idx);
tm_errcode_t ttcb_remove_region(ttcb_t* ttcb, region_id_t idx);


INLINE int ttcb_is_created(ttcb_t* ttcb) {
  return ttcb->status.state == created;
}

INLINE int ttcb_is_ready(ttcb_t* ttcb) {
  return ttcb->status.state == ready;
}

INLINE int ttcb_is_interrupted(ttcb_t* ttcb) {
  return ttcb->status.interrupted != 0;
}

INLINE void ttcb_invalidate(ttcb_t* ttcb) {
  ttcb->status.state = destroyed;
}

INLINE void ttcb_mark_ready(ttcb_t* ttcb) {
  ttcb->status.state = ready;
}

INLINE void ttcb_mark_interrupted(ttcb_t* ttcb) {
  ttcb->status.interrupted = 1;
}

INLINE void ttcb_mark_uninterrupted(ttcb_t* ttcb) {
  ttcb->status.interrupted = 0;
}

/**
 * Lock TTCB without concurrency checks.
 * Only use for freshly created TTCB's or if you know what you're doing.
 */

INLINE void ttcb_force_lock(ttcb_t* ttcb) {
  ttcb->status.locked = 1;
}

/**
 * Requires checked and locked ttcb
 */

INLINE void ttcb_unlock(ttcb_t* ttcb) {
  ttcb->status.locked = 0;
}

#endif //TTCB_H
