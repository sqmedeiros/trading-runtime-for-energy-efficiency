CAP_BACKEND ?= $(if $(shell grep -m1 AuthenticAMD /proc/cpuinfo),ryzenadj,raplcap)

ifeq ($(CAP_BACKEND),ryzenadj)
CAP_FLAGS := -DUSE_RYZENADJ
CAP_LIBS := -lryzenadj
else
CAP_LIBS := $(shell pkg-config --libs --static raplcap-powercap)
endif

main : main.c rapl.o sensors.o
	gcc -O2 $(CAP_FLAGS) -o main main.c rapl.o sensors.o $(CAP_LIBS) -lm
	chmod 777 main

rapl.o : rapl.c rapl.h
	gcc -O2  -c rapl.c  -lm

sensors.o : sensors.c sensors.h
	gcc -O2  -c sensors.c  -lm

clean:
	rm -rf main main.o rapl.o sensors.o/*
