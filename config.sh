_RISCV_XLEN=64

if [ "x$RISCVBASE" = "x" ]
then
  echo "Please set the RISCVBASE environment variable to your preferred install path."
  exit 1
fi

if [ "x$RISCV_XLEN" = "x" ]
then
  echo "Please set RISCV_XLEN to 32 or 64 bits."
  exit 1
fi

export RISCV=${RISCVBASE}/${RISCV_XLEN}
export RISCVPATH=${RISCV}/bin
export TIMBER=riscv${RISCV_XLEN}-spike
export PATH=${RISCVPATH}:${PATH}

if [[ "${VERBOSE}" -gt "0" ]]; then
  set -x
fi
