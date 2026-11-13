# echo 1  -> core is ON
# echo 0  -> core is OFF
CPUS=$(nproc --ignore=1)
for i in $(seq 1 "$CPUS")
do
  echo "Turning off /sys/devices/system/cpu/cpu$i/online"
  sudo echo 1 > "/sys/devices/system/cpu/cpu$i/online"
done
