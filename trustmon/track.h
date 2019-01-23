// See LICENSE for license details.

#ifndef TRACK_H
#define TRACK_H

#include "trustmon.h"
#include "ttcb.h"

tm_errcode_t track_region(region_t* region);
tm_errcode_t untrack_region(region_t* region);
tm_errcode_t track_ttcb(ttcb_t* ttcb);
tm_errcode_t untrack_ttcb(ttcb_t* ttcb);
tm_errcode_t check_region_overlap(region_t* region);
tm_errcode_t check_shm_offer(ttcb_t* ttcb, unsigned char* owner_eid, region_t* region);

tm_errcode_t global_track_lock();
void global_track_unlock();


#endif //TRACK_H
