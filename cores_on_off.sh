# echo 1  -> core is ON
# echo 0  -> core is OFF
sudo echo 0 > /sys/devices/system/cpu/cpu1/online 
sudo echo 0 > /sys/devices/system/cpu/cpu2/online 
sudo echo 0 > /sys/devices/system/cpu/cpu3/online
