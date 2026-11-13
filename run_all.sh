#!/usr/bin/env bash
# Stress CPU to ensure good threshold
stress-ng --cpu $(nproc) --cpu-load 80 --timeout 180s

sleep 10

# Run 20 times without perf
echo "Running measure.sh without perf"
./measure.sh 0 20

sleep 60

# Stress CPU to ensure good threshold
stress-ng --cpu $(nproc) --cpu-load 80 --timeout 180s

sleep 10

# Run 20 times with perf
echo "Running measure.sh without perf"
./measure.sh 1 20

sleep 60

echo "Turning off cores and running again"

sudo ./cores_on_off.sh

stress-ng --cpu $(nproc) --cpu-load 80 --timeout 180s

sleep 10

# Run 20 times without perf
echo "Running measure.sh without perf"
./measure.sh 0 20

sleep 60

# Stress CPU to ensure good threshold
stress-ng --cpu $(nproc) --cpu-load 80 --timeout 180s

sleep 10

# Run 20 times with perf
echo "Running measure.sh with perf"
./measure.sh 1 20




sudo reboot
