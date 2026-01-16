#!/bin/bash
cd `dirname $0`
echo MAKE .....................
make clean
clang-format --style=Chromium -i *.c *.h
make all
echo RUN ......................
./bmicalc
echo RC=$?
sleep 2