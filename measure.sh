#!/bin/bash
NTIMES=2
USE_PERF=1
OUTPUT_FILE="measurements-perf.csv"
MEASUREMENTS_FILE="measurements.csv"

#This code was tested on Linux Ubuntu Server 22.04.3 LTS
#Compile sensors wich will be used to calculate cool temperature
cd RAPL
gcc -shared -o sensors.so sensors.c
cd ..

#Update the temperature value
cd Utils/
python3 temperatureUpdate.py

#Update the number of times the program will run on each case TODO PRECISO ATUALIZAR ISTO PARA TODOS OS PROGRAMAS
for language in "../Languages"/*; do
    for program in "$language"/*; do
        if [ -d "$program" ]; then
            makefile_path="$program/Makefile"
            if [ -f "$makefile_path" ]; then
                python3 ntimesUpdate.py "$NTIMES" "$makefile_path"
            else
                echo "Makefile not found: $makefile_path"
            fi
        fi
    done
done
cd ..


echo "Language,Program,PowerLimit,Package,Core,GPU,DRAM,Time,Temperature,Memory,Perf" > $OUTPUT_FILE

# Loop over power limit values
for limit in -1 2 10 15 25
    do
    cd Utils/
    python3 raplCapUpdate.py $limit $USE_PERF ../RAPL/main.c
    cd ..
    #Make RAPL lib
    cd RAPL/
    rm sensors.so
    make
    cd ..

    for language in "Languages"/*; do
        for program in "$language"/*; do
            if [ -d "$program" ]; then
                makefile_path="$program/Makefile"
                if [ -f "$makefile_path" ]; then
                    cd $program
                    make compile
                    make measure
    
                    # Specify the input file name
                    file=$MEASUREMENTS_FILE
                    tail -n +2 "$file" >> ../../../"$OUTPUT_FILE";
                    make clean
                    cd ../../..
                else
                    echo "Makefile not found: $makefile_path"
                fi
           fi
        done
    done
done

cd RAPL/
make clean
cd ..

#sudo reboot
