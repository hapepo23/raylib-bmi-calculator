#!/bin/bash
cd `dirname $0`
echo RUN ......................
./bmicalc
echo RC=$?
echo WAIT .....................
sleep 2
