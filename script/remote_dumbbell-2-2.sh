#!/bin/bash
types="pfc cbfc ptpfc cbpfc"
for t in $types
do
    ./waf --run "scratch/pfc-cyq ns3_config/remote/dumbbell-2-2/$t/config.json $1" > /tmp/ns3_remote_dumbbell-2-2_$1_$t.log 2>&1 &
done
wait
