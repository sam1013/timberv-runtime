#!/usr/bin/python

import pandas as pd
import numpy as np
import os
import sys
import pdb
from scipy.stats.mstats import gmean

def loadcsv(name):
    return pd.read_csv(name, header=None, sep="=", index_col=0)

def out(matrix, name):
  try:
      os.mkdir("data")
  except:
      pass
  if matrix is not None:
    matrix.axes[1].name = "Name"
    #mmodel = addmodel(matrix).transpose()
    mt = matrix.transpose()
    #mt.to_latex("data/" + name + ".tex")
    mt.to_csv("data/" + name + ".csv", float_format='%.1f')
    print ""
    print "Benchmark " + name
    print mt

def loadbench(name, meas=-1, si=1, hi=0, ch=0):
    full = name
    if meas >= 0:
      full = full + "-M" + str(meas)
    if si == 1:
      full = full + "-SI"
    if hi == 1:
      full = full + "-HI"
    if ch == 1:
      full = full + "-CH"
    full = full + "-short.hist"
    obj = loadcsv(full)
    #obj.axes[0].name = "Name"
    return obj

def loadbenchfull(name, meas=-1, si=1, hi=0, ch=0):
    global nop
    global model

    obj = loadbench(name, meas, si, hi, ch)
    obj -= nop
    #s1 = riscvmodel.dot(obj)
    #s2 = timbermodel.dot(obj)
    #overhead = (s2 / s1) - 1

    #s1 = s1.rename({1: "sum-riscv"}, axis='index')
    #s2 = s2.rename({1: "sum-timber"}, axis='index')
    #overhead = overhead.rename({1: "overhead"}, axis='index')
    #obj = pd.concat([obj, s1, s2, overhead])

    return obj #.rename({1: name}, axis='columns')

def addmodel(matrix):
    global timbermodel
    global timberwcmodel
    global riscvmodel

    sr = riscvmodel.dot(matrix)
    st = timbermodel.dot(matrix)
    stwc = timberwcmodel.dot(matrix)

    ot = (st / sr) - 1
    otwc = (stwc / sr) - 1

    sr = sr.rename({1: "sum-riscv"}, axis='index')
    st = st.rename({1: "sum-timber"}, axis='index')
    stwc = stwc.rename({1: "sum-timberwc"}, axis='index')
    ot = ot.rename({1: "%"}, axis='index')
    otwc = otwc.rename({1: "%"}, axis='index')

    return pd.concat([matrix, sr, st, ot, stwc, otwc])

def model(matrix, model):
    return model.dot(matrix)

def add_gmean(matrix):
    m = gmean(matrix, axis=1)
    matrix.insert(len(matrix.columns), "geo-mean", m)

def beautify(name):
    if name.startswith("B_"):
      bname = name[2:]
    else:
      bname = name
    bname = bname.replace("_", "-") # for latex compatibility
    # Make names shorter
    bname = bname.replace("complex", "cmplx")
    bname = bname.replace("sglib-array", "sglib-")
    bname = bname.replace("listinsertsort", "listinssort")
    return bname.lower()

def concat(arr, names):
    assert(len(arr) == len(names))
    for i in range(0, len(arr)):
        if names[i] is not None:
            arr[i] = arr[i].rename({1: names[i]}, axis='index')
    return pd.concat(arr, axis=0, sort=None)

def concatt(arr, names):
    assert(len(arr) == len(names))
    for i in range(0, len(arr)):
        if names[i] is not None:
            arr[i] = arr[i].rename({1: names[i]}, axis='columns')
    return pd.concat(arr, axis=1)

def convert_percent(matrix, colname):
    c = matrix.transpose()[colname]
    c = (c - 1) * 100
    matrix.transpose()[colname] = c

timbermodel = loadcsv("timber.model").transpose()
timberwcmodel = loadcsv("timberwc.model").transpose()
riscvmodel = loadcsv("riscv.model").transpose()

encls_tests = [
"B_ECREATE",
"B_EADD0", # runtime increases with no. of ranges/ttcb's to check. Hashing costs stay the same.
           # The order of regions matters. E.g. appending vs. prepending
#"B_EADD1",
#"B_EADD2",
#"B_EADD3", # slightly higher runtime due to check_region_overlap
#"B_EDATA0", # add 0 words from region[0] to start of region
#"B_EDATA1", # add 0 words from region[0]
"B_EDATA2", # add 1 word from region[0]
#"B_EDATA3", # add 1 word from region[0]
#"B_EENTRY0",
"B_EENTRY1", # meas * #entries
#"B_EENTRY2",
#"B_EENTRY3",
"B_EINIT",
"B_ELOAD",
"B_EDESTROY", ]

encls_names = [
"create-enclave",
"add-region", # runtime increases with no. of ranges/ttcb's to check. Hashing costs stay the same.
           # The order of regions matters. E.g. appending vs. prepending
#"B_EADD1",
#"B_EADD2",
#"B_EADD3", # slightly higher runtime due to check_region_overlap
#"B_EDATA0", # add 0 words from region[0] to start of region
#"B_EDATA1", # add 0 words from region[0]
"add-data", # add 1 word from region[0]
#"B_EDATA3", # add 1 word from region[0]
#"B_EENTRY0",
"add-entries", # meas * #entries
#"B_EENTRY2",
#"B_EENTRY3",
"init-enclave",
"load-enclave",
"destroy-enclave", ]

# Enclu tests
enclu_tests = [
"B_EGETKEY",
"B_ESHMOFFER",
"B_ESHMACCEPT",
"B_ESHMCLOSE",
"B_EAEX",
"B_ERESUME"]

enclu_names = [
"get-key",
"shm-offer",
"shm-accept",
"shm-release",
"interrupt-enclave",
"resume-enclave" ]

# Other tests
other_tests = [
"B_SYSCALL",# m-mode trap delegation latency: from issuing ecall to the S-mode trap handler
"B_ENCLS",  # from call to encls function
"B_ENCLSR", # from encls function to return site
"B_ENCLU",  # from call to enclu function, including m-mode delegation, interrupt dispatching and enclu dispatching
"B_ENCLUR" ]# from enclu function to return site

eocall_tests = [
"B_ECALL",
"B_ECALLR",
"B_OCALL",
"B_OCALLR" ]

eocall_names = [
"TUenter",
"TUleave",
"ocall",
"ocall return" ]

# Load beebs benchmarks in variable beebs_tests and beebs_hitests
execfile("beebs/bench.pyinc")
# Add coremark to beeps
beebs_tests.append("B_COREMARK")

os.chdir('results')

try:
  nop = loadbench("B_NOP", si=1, ch=1)
except:
  print "Run B_NOP first!"
  sys.exit(1)

if len(sys.argv) >= 3:
  # Only work on files passed as arguments
  obj1 = loadcsv(sys.argv[1])
  obj2 = loadcsv(sys.argv[2])

  obj1r = model(obj1, riscvmodel)
  obj1t = model(obj1, timbermodel)
  obj1w = model(obj1, timberwcmodel)
  obj2r = model(obj2, riscvmodel)
  obj2t = model(obj2, timbermodel)
  obj2w = model(obj2, timberwcmodel)

  matrix = concat([obj1r, obj2r, (obj2r/obj1r), obj1t, obj2t, (obj2t/obj1t), obj1w, obj2w, (obj2w/obj1w)],
    [ "risc-v 1", "risc-v 2", "%", "timber 1", "timber 2", "%", "timberwc 1", "timberwc 2", "%",])
  print matrix.transpose()
  sys.exit(0)

try:
  bsyscall = loadbench("B_SYSCALL", si=1, ch=1)
  bencls   = loadbench("B_ENCLS", si=1, ch=1)
  benclsr  = loadbench("B_ENCLSR", si=1, ch=1)
  benclu   = loadbench("B_ENCLU", si=1, ch=1)
  benclur  = loadbench("B_ENCLUR", si=1, ch=1)
except:
  print "Run encls/enclu first!"
  sys.exit(1)

encls = concatt([bencls, benclsr], ["TSenter", "TSleave"])
# no measurement overhead
enclsm = concatt([nop-nop, nop-nop], ["TSenter", "TSleave"])
prev = None
prevname = ""
i = -1
for name in encls_tests:
    i += 1
    try:
      objm = loadbenchfull(name, meas=1, si=1, ch=1)
      obj  = loadbenchfull(name, meas=0, si=1, ch=1)
    except:
        print "Skipping " + name
        continue
    obj = obj - bencls - benclsr
    objm = objm - bencls - benclsr
    meas = objm - obj

    dfn = obj.rename({1: beautify(encls_names[i])}, axis='columns')
    dfnm = meas.rename({1: beautify(encls_names[i])}, axis='columns')
    if enclsm is None:
        enclsm = dfnm
    else:
        enclsm = pd.concat([enclsm, dfnm], axis=1)
    # Join to full matrix
    if encls is None:
        encls = dfn
    else:
        encls = pd.concat([encls, dfn], axis=1)

enclu = concatt([bsyscall, benclu - bsyscall, benclur],
              ["TSyscall", "TSyscall dispatch", "TSyscall return"])
# No measurement overhead
enclum = concatt([nop-nop, nop-nop, nop-nop],
              ["TSyscall", "TSyscall dispatch", "TSyscall return"])
i = -1
for name in enclu_tests:
    i += 1
    meas = nop - nop # Assume zero measurement overhead
    try:
      obj = loadbenchfull(name, meas=-1, si=1, ch=1)
    except:
      # Try meas=0 and meas=1 (needed for EGETKEY)
      try:
          obj = loadbenchfull(name, meas=0, si=1, ch=1)
          objm = loadbenchfull(name, meas=1, si=1, ch=1)
          meas = objm - obj
      except:
          print "Skipping " + name
          continue
    if name in ["B_ERESUME", "B_EAEX"]:
        print "Processing eresume/eaex"
        obj = obj - bsyscall
    else:
        obj = obj - benclu - benclur
    dfn = obj.rename({1: beautify(enclu_names[i])}, axis='columns')
    dfnm = meas.rename({1: beautify(enclu_names[i])}, axis='columns')
    # Join to full matrix
    if enclu is None:
        enclu = dfn
    else:
        enclu = pd.concat([enclu, dfn], axis=1)
    if enclum is None:
        enclum = dfnm
    else:
        enclum = pd.concat([enclum, dfnm], axis=1)

# Add ecall/ocall benchmarks without considering ENCLU/ENCLUR overhead
i = -1
for name in eocall_tests:
    i += 1
    meas = nop - nop # Assume zero measurement overhead
    try:
      obj = loadbenchfull(name, meas=-1, si=1, ch=1)
    except:
      print "Skipping " + name
      continue
    dfn = obj.rename({1: eocall_names[i]}, axis='columns')
    dfnm = meas.rename({1: eocall_names[i]}, axis='columns')
    enclu = pd.concat([enclu, dfn], axis=1)
    enclum = pd.concat([enclum, dfnm], axis=1)

enclst = model(encls, timbermodel)
enclsw = model(encls, timberwcmodel)
enclsmt = model(enclsm, timbermodel)
enclsmw = model(enclsm, timberwcmodel)
enclut = model(enclu, timbermodel)
encluw = model(enclu, timberwcmodel)
enclumt = model(enclum, timbermodel)
enclumw = model(enclum, timberwcmodel)

enclu_model = concat([enclut, enclumt, encluw, enclumw], [ "timber-v", "timber-v hashing", "timber-v-wc", "timber-v-wc hashing" ])
encls_model = concat([enclst, enclsmt, enclsw, enclsmw], [ "timber-v", "timber-v hashing", "timber-v-wc", "timber-v-wc hashing" ])
encl_full = pd.concat([encls_model, enclu_model], axis=1, sort=False)
out(encl_full, "encl")

beebssi0 = None
beebssi1 = None
beebsch1 = None
for name in beebs_tests:
    try:
      obj = loadbenchfull(name, si=0)
      si = loadbenchfull(name, si=1)
      ch = loadbenchfull(name, si=0, ch=1)
    except Exception, e:
        print "Skipping " + name
        print e
        continue

    dfn = obj.rename({1: beautify(name)}, axis='columns')
    # Join to full matrix
    if beebssi0 is None:
        beebssi0 = dfn
    else:
        beebssi0 = pd.concat([beebssi0, dfn], axis=1)

    dfn = si.rename({1: beautify(name)}, axis='columns')
    # Join to full matrix
    if beebssi1 is None:
        beebssi1 = dfn
    else:
        beebssi1 = pd.concat([beebssi1, dfn], axis=1)

    dfn = ch.rename({1: beautify(name)}, axis='columns')
    # Join to full matrix
    if beebsch1 is None:
        beebsch1 = dfn
    else:
        beebsch1 = pd.concat([beebsch1, dfn], axis=1)

beebssi0.axes[1].name = "Name"
beebssi1.axes[1].name = "Name"
beebsch1.axes[1].name = "Name"

beebshi0 = None
beebshi1 = None
for name in beebs_hitests:
    try:
      obj = loadbenchfull(name, si=0, hi=0)
      hi = loadbenchfull(name, si=0, hi=1)
    except Exception, e:
        print "Skipping " + name
        print e
        continue

    dfn = obj.rename({1: beautify(name)}, axis='columns')
    # Join to full matrix
    if beebshi0 is None:
        beebshi0 = dfn
    else:
        beebshi0 = pd.concat([beebshi0, dfn], axis=1)

    dfn = hi.rename({1: beautify(name)}, axis='columns')
    # Join to full matrix
    if beebshi1 is None:
        beebshi1 = dfn
    else:
        beebshi1 = pd.concat([beebshi1, dfn], axis=1)

beebshi0.axes[1].name = "Name"
beebshi1.axes[1].name = "Name"



# no stack interleaving
si0r = model(beebssi0, riscvmodel)
si0t = model(beebssi0, timbermodel)
si0w = model(beebssi0, timberwcmodel)
# with stack interleaving
si1r = model(beebssi1, riscvmodel)
si1t = model(beebssi1, timbermodel)
si1w = model(beebssi1, timberwcmodel)
# with code hardening
ch1r = model(beebsch1, riscvmodel)
ch1t = model(beebsch1, timbermodel)
ch1w = model(beebsch1, timberwcmodel)

# no heap interleaving
hi0r = model(beebshi0, riscvmodel)
hi0t = model(beebshi0, timbermodel)
hi0w = model(beebshi0, timberwcmodel)
# with heap interleaving
hi1r = model(beebshi1, riscvmodel)
hi1t = model(beebshi1, timbermodel)
hi1w = model(beebshi1, timberwcmodel)

beebssi0_model = concat([si0r, si0t, (si0t/si0r), si0w, (si0w/si0r)],
  [ "risc-v", "timber", "tpc", "timber-wc", "tpc-wc"])

beebssi1_model = concat([si0t, si1t, (si1t/si0t), si0w, si1w, (si1w/si0w)],
  [ "timber", "timber-si", "tsipc", "timber-wc", "timber-si-wc", "tsipc-wc"])

beebsch1_model = concat([si0t, ch1t, (ch1t/si0t), si0w, ch1w, (ch1w/si0w)],
  [ "timber", "timber-ch", "tchpc", "timber-wc", "timber-ch-wc", "tchpc-wc"])

beebshi1_model = concat([hi0t, hi1t, (hi1t/hi0t), hi0w, hi1w, (hi1w/hi0w)],
  [ "timber", "timber-hi", "thipc", "timber-wc", "timber-hi-wc", "thipc-wc"])

# The '%' columns contain ratio instead of overhead (ratio - 1) because:
#   gmean over ratio == ratio over gmean
#   overhead could become zero, in which case gmean fails
add_gmean(beebssi0_model)
add_gmean(beebssi1_model)
add_gmean(beebsch1_model)
add_gmean(beebshi1_model)

out(beebssi0, "beebs-raw")
out(beebssi1, "beebs-si-raw")
out(beebsch1, "beebs-ch-raw")
out(beebshi1, "beebs-hi-raw")

convert_percent(beebssi0_model, "tpc")
convert_percent(beebssi0_model, "tpc-wc")
convert_percent(beebssi1_model, "tsipc")
convert_percent(beebssi1_model, "tsipc-wc")
convert_percent(beebsch1_model, "tchpc")
convert_percent(beebsch1_model, "tchpc-wc")
convert_percent(beebshi1_model, "thipc")
convert_percent(beebshi1_model, "thipc-wc")

out(beebssi0_model, "beebs")
out(beebssi1_model, "beebs-si")
out(beebsch1_model, "beebs-ch")
out(beebshi1_model, "beebs-hi")
