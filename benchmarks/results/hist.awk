BEGIN { 
  FS = " "
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

# loadct
/\[HIST\] (l[a-z]+ct)/ {
  debugprint($0)
  ldct+=$3
  next
}

# load
/\[HIST\] (lb|lh|lw|ld|flw|fld)/ {
  debugprint($0)
  ld+=$3
  next
}

# storect
/\[HIST\] (s[a-z]+ct)/ {
  debugprint($0)
  stct+=$3
  next
}

# store
/\[HIST\] (sb|sh|sw|sd|fsw|fsd)/ {
  debugprint($0)
  st+=$3
  next
}

# load immediate
/\[HIST\] (lui|auipc|fmv|fcvt|fsgn|flt|fle|feq|fcmp|fclass)/ {
  debugprint($0)
  li+=$3
  next
}

# csr
/\[HIST\] (csr)/ {
  debugprint($0)
  csr+=$3
  next
}

# arithmetic
/\[HIST\] (add|sub|fadd|fsub)/ {
  debugprint($0)
  arith+=$3
  next
}

# logic
/\[HIST\] (and|or|sll|slt|sra|srl|xor)/ {
  debugprint($0)
  logic+=$3
  next
}

# mul
/\[HIST\] (mul|fmul)/ {
  debugprint($0)
  mul+=$3
  next
}

# div
/\[HIST\] (div|rem|fdiv)/ {
  debugprint($0)
  div+=$3
  next
}

# branching
/\[HIST\] (beq|bge|blt|bne|jal)/ {
  debugprint($0)
  branch+=$3
  next
}

# syscall/ret
/\[HIST\] (sret|mret|dret|ebreak|ecall)/ {
  debugprint($0)
  sys+=$3
  next
}

/\[HIST\] (stall)/ {
  debugprint($0)
  stall+=$3
  next
}

# tag
# old: stag|ltag
/\[HIST\] (ltt)/ {
  debugprint($0)
  tag+=$3
  next
}

/\[HIST\] (amo)/ {
  debugprint($0)
  amo+=$3
  next
}

# other
/\[HIST\] (wfi|sfence_vma|fence|sc_d|sc_w|lr_d|lr_w)/ {
  debugprint($0)
  other+=$3
  next
}

/\[HIST\]/ {
  print "unmatched insns: " $0
  assert(0, "Unknown hist element: " $0)
}

function printhist(name, cnt)
{
  printf "%s=%d\n", name, cnt
}

END {
  if (_assert_exit) {
    print "ASSERT failed"
    exit 1
  }

  printhist("ld", ld+tag)
  printhist("ldct", ldct)
  printhist("st", st)
  printhist("stct", stct)
  printhist("reg", li+csr+arith+logic+branch+sys)
  printhist("mul", mul)
  printhist("div", div)
  printhist("stall", stall)
  printhist("other", amo+other)
  exit 0
}
