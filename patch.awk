# This script patches loads and stores with tag-checked variants. E.g. 
#    ld   a0, 0(sp)      -->   ldct st, a0, 0(sp)     # Check if tag==st
#    sd   x0, 0(sp)      -->   sdct st, n, x0, 0(sp)  # Check if tag==st. In that case, store the new tag 'n'
#
# In addition, it manipulates the function proloque and epiloque to prepare
# and cleanup the used stack frame. 
# Function proloques are detected whenever decrementing 'sp', e.g.
#    addi  sp, sp, -8
# Function epiloques are detected whenever incrementing 'sp', e.g.
#    addi  sp, sp, 8
# The function proloque is extended to change the tag from NTAG to STAG
# The function epiloque is extended to revert the tag from STAG to NTAG 
# and clear the stack frame to 0.
#
# Command line parameters:
# -v SI=<1|0>   Enable stack interleaving, patching stack allocation and deallocation
# -v CH=<1|0>   Enable code hardening, patching all loads/stores with their checked variants
# -v STAG=<tag> Secure tag used for checked load/stores. Can be one of n|c|st|ut.
# -v mixed=1    Optional: Input file has mixed content (normal and secure). 
#               Only secure content is patched. Secure content is detected 
#               by a ".section" entry containing the string ".secure" in the object's header.
#               If another section is entered (e.g. .text or .section bla), patching is stopped again until the next
#               ".secure" section is detected.
# -v debug=1    Optional: Enable debug output in form of assembler comments

BEGIN { 
  FS = ","
  # Normal tag
  NTAG="n"
  # Secure tag
  assert(STAG ~ /n|c|st|ut/, "Wrong or no secure tag (STAG) specified. Provide 'awk -v STAG=<tag>' with <tag> being one of [n|c|st|ut]'")
  if (RISCV_XLEN == 64) {
    REGBYTES = 8
  }
  if (RISCV_XLEN == 32) {
    REGBYTES = 4
  }
  assert(REGBYTES == 4 || REGBYTES == 8, "No or wrong RISCV_XLEN specified. Provide 'awk -v RISCV_XLEN=32|64'")
  if (REGBYTES == 8) {
    SCT = "sdct"
    LCT = "ldct"
  } else {
    SCT = "swct"
    LCT = "lwct"
  }
  
  # Enable per default
  if (length(SI) == 0) {
    SI = 1
  }
  if (length(CH) == 0) {
    CH = 1
  }
  
  if (mixed) {
    header = 0
    secure = 0
  } else {
    secure = 1
  }
}

function assert(condition, string)
{
    if (! condition) {
        printf("%s:%d: assertion failed: %s\n",
            FILENAME, FNR, string) > "/dev/stderr"
        _assert_exit = 1
        exit 1
    }
}

function debugprint(string)
{
  if (debug) {
    print string
  }
}

# Patch function proloque and epiloque
#
# A function proloque is identified by
# 	add	sp,sp,-<fs>
#
# A function epiloque is identified by
# 	add	sp,sp,<fs>
#
# where <fs> is the frame size
#
# We prepare the whole frame by inserting
# checked stores, changing the frame from normal tag N to secure tag ST
# and vice versa
#

mixed && ( $0 ~ /^\s+\.section/ || $0 ~ /^\s+\.text/ || $0 ~ /^\s+\.data/ ) {
  debugprint("#HEADER-START")
  secure = 0
}

mixed && !secure && /\.section.*\.secure/ {
  debugprint("#SECURE")
  secure = 1
}

####################
# Stack Interleaving
####################

# Sometimes, the stack pointer is increased by a temporary register, say t0. E.g.:
#   li   t0, 2048
#   add  sp, sp, t0
# Sometimes, even with an addition on t0, e.g.:
#   li   t0, 2048
#   add  t0, t0, 1024
#   add  sp, sp, t0
# To handle these cases, we detect those sequences and store the 
# temporary register inside 'lireg' and its offset inside 'lioff'

# On 'li', save register in 'lireg' and offset in 'lioff' for potential later usage
SI && secure && /\s+li\s+/ {
  lireg = gensub(/\s+li\s+([a-z0-9]+),.*/, "\\1", "g", $0)
  #print "# li reg = " lireg
  lioff = gensub(/\s+li\s+[a-z0-9]+,([-+]?[0-9]+).*/, "\\1", "g", $0)
  #print "# li off = " lioff
  liline = FNR
}

# If previous instruction was a 'li' and we now have an add on the same 'lireg'
# increase/decrease 'lioff' according to the add.
SI && secure && /\s+add[i]?\s+[a-z0-9]+,[a-z0-9]+,([-+]?[0-9]+)/ {
  addreg1 = gensub(/\s+add[i]?\s+([a-z0-9]+),.*/, "\\1", "g", $0)
  addreg2 = gensub(/\s+add[i]?\s+[a-z0-9]+,([a-z0-9]+),.*/, "\\1", "g", $0)
  addoff =  gensub(/\s+add[i]?\s+[a-z0-9]+,[a-z0-9]+,([-+]?[0-9]+).*/, "\\1", "g", $0)
  #printf "#add %s = %s + %s\n", addreg1, addreg2, addoff
  if (liline == FNR - 1 && addreg1 == lireg && addreg2 == lireg) {
    lioff = lioff + addoff
    #printf "#new offset = %d\n", lioff
    liline = FNR
  }
}

# Matches 'add sp,sp,lireg'
# Previous instructions have to be 'li' on 'lireg' with optional 'add' on 'lireg'
# The value of 'lireg' is stored in 'lioff'
SI && secure && /\s+add\s+sp,sp,[a-z]+[0-9]+/ {
  addreg = gensub(/\s+add\s+sp,sp,([a-z]+[0-9]+).*/, "\\1", "g", $0)
  assert(addreg == lireg, "Invalid 'add,sp,sp,reg' detected without corresponding 'li,reg,offset'")
  assert(liline == FNR - 1, "Invalid 'add,sp,sp,reg' detected without corresponding 'li,reg,offset'")
  
  assert(!(lioff % REGBYTES), "unaligned frame size")
  if (lioff < 0) {
    printf("L%d:\n", FNR)
    printf "\taddi\tsp,sp,%d\n", -REGBYTES
    printf "\t%s\t%s,%s,x0,%d(sp)\n", SCT, NTAG, STAG, 0
    printf "\tadd\t%s,%s,%d\n", addreg, addreg, REGBYTES
    printf "\tbnez\t%s,L%d\n", addreg, FNR
  } else {
    printf("L%d:\n", FNR)
    printf "\t%s\t%s,%s,x0,%d(sp)\n", SCT, STAG, NTAG, 0
    printf "\taddi\tsp,sp,%d\n", REGBYTES
    printf "\tadd\t%s,%s,%d\n", addreg, addreg, -REGBYTES
    printf "\tbnez\t%s,L%d\n", addreg, FNR
  }
  printf "\tli\t%s,%d\n", addreg, lioff
  next
}

# addi sp,sp,offset
SI && secure && /\s+add\s+sp,sp,[-+]?[0-9]+/ {
  fs=$3
  assert(!(fs % REGBYTES), "unaligned frame size")
  
  if (fs < 0) {
    # Function proloque
    do {
      if (fs <= -128) {
        ffs=-128 # we can only handle 128 bytes at once
      } else {
        ffs=fs   # last block
      }
      printf "\taddi\tsp,sp,%d\n", ffs
      do {
        ffs += REGBYTES
        printf "\t%s\t%s,%s,x0,%d(sp)\n", SCT, NTAG, STAG, -ffs
      } while (ffs < 0)
      fs += 128
    } while (fs < 0)
  } else {
    # Function epiloque
    do {
      if (fs >= 128) {
        ffs=128 # we can only handle 128 bytes at once
      } else {
        ffs=fs   # last block
      }
      copyffs=ffs
      do {
        ffs -= REGBYTES
        printf "\t%s\t%s,%s,x0,%d(sp)\n", SCT, STAG, NTAG, ffs
      } while (ffs > 0)
      printf "\taddi\tsp,sp,%d\n", copyffs
      fs -= 128
    } while (fs > 0)
  }
  next
}

####################
# Code Hardening
####################

# Patch loads
CH && secure && /\s+l(b|h|w|d|bu|hu|wu)\s+/ {
  debugprint("#PATCHING " $0)
  if ($0 ~ /\(.*\)/ ) {
    /* standard load relative to register */
    split($0,arr,/[,\(\)]/)
    opc = arr[1]
    offset = arr[2]
    dstreg = arr[3]

    # Extract target register
    treg = gensub(/\s+[a-z]+\s+([a-z]+[0-9]*).*/, "\\1", "g", opc)

    # Original opc reconstructed
    parsed = opc "," offset "(" dstreg ")"
    assert($0 == parsed, "Parsing error!\n" "ORIG:   " $0 ".\nPARSED: " parsed ".")

    # Patch the opcode
    opc =  gensub(/(l(b|h|w|d|bu|hu|wu))(\s+)/, "\\1ct\\3" STAG ",", "g", opc)

    # Fix potential offset over/underflows
    eo = offset # effective offset
    if (offset >= 512 || offset <= -512) {
      # If original load uses offset -2048, we cannot simply
      # addi +2048, which overflows the immediate by one. Hence, 
      # shift an offset of 8 bytes from addi into ld.
      if (offset > 0) {
        eo = 8
      } else {
        eo = -8
      }

      # Manually increase base in dstreg
      printf "\taddi\t%s,%s,%d\n", dstreg, dstreg, offset - eo
      print opc "," eo "(" dstreg ")"
      if (dstreg != treg) {
        # Manually recover original base in dstreg
        printf "\taddi\t%s,%s,%d\n", dstreg, dstreg, -offset + eo
      }
    } else {
      print opc "," offset "(" dstreg ")"
    }
  } else {
    /* load macro */
    print gensub(/(l(b|h|w|d|bu|hu|wu))(\s+)/, "\\1ct\\3" STAG ",", "g", $0)
  }
  next
}

# Patch stores
CH && secure && /\s+s(b|h|w|d)\s+/ {
  debugprint("#PATCHING " $0)
  if ($0 ~ /\(.*\)/ ) {
    /* standard store relative to register */
    split($0,arr,/[,\(\)]/)
    opc = arr[1]
    offset = arr[2]
    dstreg = arr[3]

    # Extract target register
    treg = gensub(/\s+[a-z]+\s+([a-z]+[0-9]*).*/, "\\1", "g", opc)
    if ((dstreg == "zero" && treg == "zero") || (dstreg == "x0" && treg == "x0") ) {
      # Used for compiler-introduced cfi_restore constructs. Ignore.
      print $0
      next
    }
    assert(dstreg != treg, "Cannot apply patch since both registers are the same!")

    # Original opc reconstructed
    parsed = opc "," offset "(" dstreg ")"
    assert($0 == parsed, "Parsing error!\n" "ORIG:   " $0 ".\nPARSED: " parsed ".")
    
    # Patch the opcode
    opc =  gensub(/(s(b|h|w|d))(\s+)/, "\\1ct\\3" STAG "," STAG ",", "g", opc)

    # Fix potential offset over/underflows
    eo = offset # effective offset of store
    if (offset >= 128 || offset <= -128) {
          # If original load uses offset -2048, we cannot simply
      # addi +2048, which overflows the immediate by one. Hence, 
      # shift an offset of 8 bytes from addi into ld.
      if (offset > 0) {
        eo = 8
      } else {
        eo = -8
      }

      # Manually increase base in dstreg
      printf "\taddi\t%s,%s,%d\n", dstreg, dstreg, offset - eo
      print opc "," eo "(" dstreg ")"
      # Manually recover original base in dstreg
      printf "\taddi\t%s,%s,%d\n", dstreg, dstreg, -offset + eo
    } else {
      print opc "," eo "(" dstreg ")"
    }
  } else {
    /* store macro */
    print gensub(/(s(b|h|w|d))(\s+)/, "\\1ct\\3" STAG "," STAG ",", "g", $0)
  }
  next
}

# Making awk print all non-matching lines too
1

END {
  if (_assert_exit) {
    print "ASSERT failed"
    exit 1
  }
  exit 0
}
