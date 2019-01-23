#!/bin/bash

./compile_timber.sh
cd beebs
./compile.sh
cd ..
make -C results -j`nproc` all
./results.py
