// See LICENSE for license details.

#ifndef MEASURE_H
#define MEASURE_H

#include "trustmon.h"
#include "ttcb.h"

tm_errcode_t measure_init(ttcb_t* ttcb);
tm_errcode_t measure_region(ttcb_t* ttcb, region_t* region);
tm_errcode_t measure_data(ttcb_t* ttcb, range_t* range);
tm_errcode_t measure_entry(ttcb_t* ttcb, const void* entry);
tm_errcode_t measure_finalize(ttcb_t* ttcb);

#endif //MEASURE_H

