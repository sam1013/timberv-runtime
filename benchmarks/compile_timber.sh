#!/bin/bash

# Compile timber benchmarks defined via B_XXX macros. The resulting 
# binaries will be stored in 'results'
#
# To compile all, run without arguments.
# To compile a single benchmark, run as follows:
#   ./compile_timber.sh <name> <meas> <si>
# where
#   <name> is the benchmark macro name, e.g. B_NOP or B_COREMARK
#   <meas> is 1 or 0 to enable/disable compilation of ENCLS hash measurement code. 
#          This allows to separate the hashing runtime from the rest. 
#          It will add "-M1" or "-M0" to the compiled binary, e.g. B_ECREATE-M0.
#          Set to -1 if not needed.
#   <si>   is 1 or 0 to enable/disable stack interleaving. If 1, 
#          a "-SI" will be added, e.g. "B_COREMARK-SI".
#          The variable STACKINTERLEAVING will be set to <si>, which 
#          allows the Makefiles to enable/disable it.
#

source ../../config.common

# Fail on error
set -e

BENCH_DIR=results
BIN_DIR=..

# $1 ... benchmark name (e.g. B_CREATE), will be defined as preprocessor symbol (e.g. -DB_ECREATE=1)
# $2 ... enable or disable encls hash measurement (1/0) or does not apply (-1)
# $3 ... enable or disable stack interleaving (1/0)
# $4 ... enable or disable code hardening (1/0)
function compile_timber_benchmark {
  NAME=$1
  MEAS=$2
  SI=$3
  CH=$4
  if [[ "$MEAS" -ge "0" ]]; then
    NAME=${NAME}-M$2
  fi
  if [[ "$SI" -eq "1" ]]; then
    NAME=${NAME}-SI
  fi
  if [[ "$CH" -eq "1" ]]; then
    NAME=${NAME}-CH
  fi
  if ! [[ -f ${BENCH_DIR}/${NAME}.elf ]]; then
    echo "Compiling $NAME"
    eval SI=$SI CH=$CH TM_BENCH=1 BENCHTEST=$1 TM_PRINTF=0 TM_MEASURE=$2 make -C ${BIN_DIR} clean &> ${BENCH_DIR}/${NAME}.log || exit 1
    eval SI=$SI CH=$CH TM_BENCH=1 BENCHTEST=$1 TM_PRINTF=0 TM_MEASURE=$2 make -C ${BIN_DIR} &>> ${BENCH_DIR}/${NAME}.log || exit 1
    mv ${BIN_DIR}/${TIMBER}.elf ${BENCH_DIR}/${NAME}.elf
  fi
}

# $1 ... enable or disable encls hash measurement (1/0) or does not apply (-1)
# $2 ... enable or disable stack interleaving (1/0)
# $3 ... enable or disable code hardening (1/0)
# $4* ... benchmark name (e.g. B_CREATE), will be defined as preprocessor symbol (e.g. -DB_ECREATE=1)
function run_suite {
  MEAS=$1
  SI=$2
  CH=$3
  shift 3
  arr=("$@")
  for t in "${arr[@]}"; do
    compile_timber_benchmark $t $MEAS $SI $CH
  done
}

# Tests that are affected by ENCLS measurements
ENCLS_TESTS=(\
B_ECREATE \
B_EADD0 \
B_EADD1 \
B_EADD2 \
B_EADD3 \
B_EDATA0 \
B_EDATA1 \
B_EDATA2 \
B_EDATA3 \
B_EENTRY0 \
B_EENTRY1 \
B_EENTRY2 \
B_EENTRY3 \
B_EINIT \
B_EDESTROY \
B_ELOAD )

# Other tests
OTHER_TESTS=(\
B_NOP \
B_SYSCALL \
B_ENCLS \
B_ENCLSR \
B_ENCLU \
B_ENCLUR \
B_ESHMOFFER \
B_ESHMACCEPT \
B_ESHMCLOSE \
B_EAEX \
B_ERESUME \
B_ECALL \
B_ECALLR \
B_OCALL \
B_OCALLR )

if [[ "$#" -eq "0" ]]; then

  # ENCLS_TESTS and OTHER_TESTS only work with enabled stack interleaving
  # and code hardening since the app / the enclave and trustmon operate
  # on a single stack

  # ENCLS_TESTS
  # Baseline without and with hash measurement
  # ENCLS with stack interleaving and code hardening transformation
  run_suite  0 1 1 "${ENCLS_TESTS[@]}"
  run_suite  1 1 1 "${ENCLS_TESTS[@]}"

  # OTHER_TESTS with stack interleaving and code hardening transformation
  run_suite -1 1 1 "${OTHER_TESTS[@]}"

  # EGETKEY performance without and with hash measurement
  run_suite 0 1 1 B_EGETKEY
  run_suite 1 1 1 B_EGETKEY

  # coremark
  run_suite -1 0 0 B_COREMARK
  # + Stack interleaving
  run_suite -1 1 0 B_COREMARK
  # + Code hardening
  run_suite -1 0 1 B_COREMARK
else
  compile_timber_benchmark $*
fi
