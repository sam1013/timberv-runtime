#!/bin/bash
#
# This script patches an assembler file replacing normal load/store with checked-tag instructions.
#
# Since an application assembler file might mix secure and normal sections, 
# this script only patches secure sections. In order to do so, it searches 
# for portions of assembler code whose header contains the following line:
# <tab>.section<arbitrary text>.secure<arbitrary text>
#
# that is, a section that contains the text '.secure'
#
# Headers are identified by a leading tab, followed by a dot.
#
# Example which will be patched:
#
#	.section	.app.1.secure.text.b,"ax",@progbits
#	.align	3
#code:
#	add	sp,sp,-16
#	sd	ra,8(sp)     <-- will be patched

# Only those portions are patched which contain a secure section 
if [[ $# -ne 2 ]]; then
  echo "Provide <file> <tag> as arguments!"
  exit 1
fi

FILE=$1
TAG=$2
if ! [[ -f ${FILE} ]]; then
  echo "File '${FILE}' does not exist!"
  exit 1
fi

case $TAG in
n);;
c);;
ut);;
st);;
*)
  echo "Expects tag to be one of [n,c,ut,st]!"
  exit 1
  ;;
esac

FILEORIG=$1.orig
FILETMP=$1.tmp
TAG=$2

cp ${FILE} ${FILEORIG}

# $1 ... line to patch
# $2 ... expected tag
function do_patch {
  # Fill in line numbers <l>
  # Fill in tag <t>
  SED=$(sed -e "s/<l>/${1}/g; s/<t>/${2}/g" patch_template.sed)
  sed -e "$SED" -i ${FILE}
}

HEADER=0
SECURE=0
LINE=0
PATCHED=0
while IFS= read -r l; do
  LINE=$((LINE+1))
  if [[ $(echo "$l" | grep -e "^	\.") ]]; then
    # We are in the header
    if [[ "${HEADER}" == "0" ]]; then
      # A new header started
      HEADER=1
      SECURE=0
    fi
    if [[ $(echo "$l" | grep -e "^	\.section.*secure") ]]; then
      # we found a secure section
      SECURE=1
    fi
  else
    HEADER=0
    if [[ "${SECURE}" == "1" ]]; then
      do_patch $LINE $TAG
      PATCHED=1
    else
      :
    fi
  fi
done < ${FILEORIG}

exit ${PATCHED}
