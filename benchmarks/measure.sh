#!/usr/bin/env bash
USE_PERF=${1:-1}
NTIMES=${2:-20}
JAVA_PATH=/usr/lib/jvm/jdk-25.0.4-oracle-x64
OUTPUT_FILE="dacapo-measurements-perf-$USE_PERF-$(date +'%d-%m-%Y-%H:%M:%S').csv"
MEASUREMENTS_FILE="dacapo-measurements-$(date +'%d-%m-%Y-%H:%M:%S').csv"
cd ..

# Compile sensors which will be used to calculate cool temperature
cd RAPL || exit
gcc -shared -o sensors.so sensors.c
cd ..

# Update the temperature value
cd Utils
sudo python3 temperatureUpdate.py

cd ..


echo "Language,Program,PowerLimit,PSys,Package,Core,GPU,DRAM,Time(ms),User Time,System Time,Cache References,Cache Misses,Temperature,Memory,Perf" > $OUTPUT_FILE

programs_dacapo=("avrora" "batik" "biojava" "cassandra" "eclipse" "fop" "graphchi" "h2" "h2o" "jme" "jython" "kafka" "luindex" "lusearch" "pmd" "spring" "sunflow" "tomcat" "tradebeans" "tradesoap" "xalan" "zxing")

for limit in -1 2 5 10 15 25; do
    pwd
    cd Utils/
    sudo python3 raplCapUpdate.py "$limit" $USE_PERF $MEASUMENTS_FILE ../RAPL/main.c
    cd ..

    # Make RAPL lib
    cd RAPL/ || exit
    sudo make
    cd ../benchmarks
    
    # Run each Dacapo program and append results to the final CSV file
    for program in "${programs_dacapo[@]}"; do
        cd dacapo || exit
        sudo ../../RAPL/main "sudo $JAVA_PATH/bin/java -jar dacapo-23.11-MR2-chopin.jar $program" dacapo "$program" "$NTIMES"        
	if ! [ -e measurements.csv ] ; then 
          touch measurements.csv
	fi
        sudo tail -n +2 measurements.csv >> ../measurements_benchmarks.csv
        cd ..
    done
    cd ..
done

cd RAPL/ || exit
sudo make clean
cd ..
