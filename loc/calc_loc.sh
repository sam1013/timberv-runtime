#!/bin/bash

set -e
SRCDIR=tm_src
OUTDIR=${PWD}/tm_loc
IFDEF="-DTM_PRINTF=0 -DTM_BENCH=0 -DTM_MEASURE=1 -D__riscv_xlen=32"

rm -rf ${OUTDIR}
mkdir -p ${OUTDIR}

pushd ${SRCDIR}
for f in *; do
  cp --copy-contents $f ${OUTDIR}
done
popd

pushd ${OUTDIR}
for f in *; do
  unifdef -x 2 ${IFDEF} -m $f
done
popd

# Show LOC per file
sloccount --details ${OUTDIR}
# Show sum over all
sloccount ${OUTDIR}
