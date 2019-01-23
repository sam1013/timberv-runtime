// See LICENSE for license details.

#include "trustmon.h"
#include <stddef.h>
#include "ttcb.h"
#include "measure.h"
#include "sha256.h"

static unsigned char tmp_buf[TM_TMP_SIZE];

#define sha256_update_uint64_t(ttcb, data) do { \
    uint64_t temp = (uint64_t)(data); \
    sha256_update(&ttcb->hash, (const unsigned char*)&temp, 8); \
  } while(0)

#define sha256_update_relative_ptr(ttcb, ptr) do { \
    uint64_t temp = ((uintptr_t)ptr - (uintptr_t)ttcb); \
    sha256_update(&ttcb->hash, (unsigned char*)&temp, 8); \
  } while(0)

tm_errcode_t measure_init(ttcb_t* ttcb) {
  sha256_init(&ttcb->hash);
  sha256_update(&ttcb->hash, (const unsigned char*)"\0\0\0\0INIT", 8);
  sha256_update_relative_ptr(ttcb, (void*)0);     // base offset
  sha256_update_zeropad(&ttcb->hash);
  return TM_SUCCESS;
}

tm_errcode_t measure_region(ttcb_t* ttcb, region_t* region) {
  sha256_update(&ttcb->hash, (const unsigned char*)"\0\0REGION", 8);
  sha256_update_relative_ptr(ttcb, region->range.start);
  sha256_update_relative_ptr(ttcb, region->range.end);
  sha256_update_uint64_t(ttcb, region->flags);
  sha256_update_zeropad(&ttcb->hash);
  return TM_SUCCESS;
}

tm_errcode_t measure_data(ttcb_t* ttcb, range_t* range) {
  sha256_update(&ttcb->hash, (const unsigned char*)"\0\0\0\0DATA", 8);
  sha256_update_relative_ptr(ttcb, range->start);
  sha256_update_relative_ptr(ttcb, range->end);
  sha256_update_zeropad(&ttcb->hash);

  size_t size = (uintptr_t)range->end - (uintptr_t)range->start;
  unsigned char* ptr = range->start;
  while (size > 0) {
    size_t tmp_len = size > TM_TMP_SIZE ? TM_TMP_SIZE : size;
    tagged_copy_from_ut(tmp_buf, ptr, tmp_len);
    sha256_update(&ttcb->hash, tmp_buf, tmp_len); // data
    ptr += tmp_len;
    size -= tmp_len;
  }

  sha256_update_zeropad(&ttcb->hash);
  return TM_SUCCESS;
}

tm_errcode_t measure_entry(ttcb_t* ttcb, const void* entry) {
  sha256_update(&ttcb->hash, (const unsigned char*)"\0\0\0ENTRY", 8);
  sha256_update_uint64_t(ttcb, entry);
  sha256_update_zeropad(&ttcb->hash);
  return TM_SUCCESS;
}

tm_errcode_t measure_finalize(ttcb_t* ttcb) {
  sha256_final(&ttcb->hash, ttcb->identity);
  return TM_SUCCESS;
}
