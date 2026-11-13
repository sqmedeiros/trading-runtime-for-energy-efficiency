#!/usr/bin/env bash
# echo 1  -> core is ON
# echo 0  -> core is OFF

TURN_ON=${1:-0}
ALL_CPUS=$(nproc --all --ignore=1)
CPUS=$(nproc --ignore=1)

if [[ "$TURN_ON" -eq 1 ]] ; then
  for i in $(seq 1 "$ALL_CPUS"); do
    echo "Turning on /sys/devices/system/cpu/cpu$i/online"
    sudo echo 1 > "/sys/devices/system/cpu/cpu$i/online"
   done
else
  for i in $(seq 1 "$CPUS"); do
    echo "Turning off /sys/devices/system/cpu/cpu$i/online"
    sudo echo 0 > "/sys/devices/system/cpu/cpu$i/online"
  done
fi
