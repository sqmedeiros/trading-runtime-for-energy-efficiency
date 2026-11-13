/*
 *  Análise e Teste de Software
 *  João Saraiva
 *  2016-2017
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef USE_RYZENADJ
#include <ryzenadj.h>
#else
#include <powercap/powercap.h>
#include <raplcap/raplcap.h>
#endif
#include <math.h>
#include "rapl.h"
#include "sensors.h"
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>

#define TEMPERATURETHRESHOLD 55.0
#define VARIANCE 10
#define WHATTSCAP 2
#define MAX_STRING_LENGTH 500
#define MAX_COMMAND_LENGTH 500
#define MEASUREMENTS_FILE "measurements-11-11-2026-12:52:37.csv"
#define TIME_OUT_LIMIT 60*1500
#define USE_PERF 1


bool event_exists(char* event_name){
  char cmd[MAX_COMMAND_LENGTH];
  sprintf(cmd, "perf list --no-desc | grep %s", event_name);
  int out = system(cmd);
  if(WEXITSTATUS(out) != 0){
    return false;
  }

  return true;
};

struct event_candidates {
  bool energy_pkg;
  bool energy_cores;
  bool energy_gpu;
  bool energy_ram;
  bool energy_psys;
  bool amd_energy_core;
};


 struct event_candidates fetch_energy_events(){
   char out[MAX_COMMAND_LENGTH];

   struct event_candidates ec = {0};
   if(event_exists("power/energy-pkg")) ec.energy_pkg = true;
   if(event_exists("power/energy-cores")) ec.energy_cores = true;
   if(event_exists("power/energy-gpu")) ec.energy_gpu = true;
   if(event_exists("power/energy-ram")) ec.energy_ram = true;
   if(event_exists("power/energy-psys")) ec.energy_psys = true;
   if(cpu_vendor==CPU_VENDOR_AMD && event_exists("power_core/energy-core")) ec.amd_energy_core = true;

   return ec;
};



void performMeasurements(const char* command,const char* language,const char* program,int ntimes, int core);

// Structure to hold function return value and timeout flag
struct ThreadData {
    int returnValue;
};

// Global structure to communicate between threads
struct ThreadData threadData;


long fib(int n) 
{
    if (n < 1)
        return 1;
    else
        return fib(n - 1) + fib(n - 2);
}

#ifdef USE_RYZENADJ
/* Limits (W) found before capping*/
static float orig_stapm = 0, orig_fast = 0, orig_slow = 0;

static int setLimit(int (*set)(ryzen_access, uint32_t), ryzen_access ry, uint32_t mw, const char *name) {
    int err = set(ry, mw);
    if (err)
        fprintf(stderr, "%s(%u mW) failed with error %d\n", name, mw, err);
    return err;
}

static void printLimits(ryzen_access ry, const char *when) {
    if (refresh_table(ry))
        return;
    printf("Limits %s: STAPM %.1f W, fast %.1f W, slow %.1f W\n", when,
           get_stapm_limit(ry), get_fast_limit(ry), get_slow_limit(ry));
}

/*
 * The package has three limits: STAPM (Skin Temperature-Aware Power System) and slow PPT (long term) and fast PPT (short term). PPT = Package Power Tracking
 * All of them get WHATTSCAP, like the long and short raplcap limits on Intel.
 */
int initializeRyzenadj(ryzen_access *ry) {
    // printf("Running with CAP (ryzenadj)\n");
    uint32_t mw = WHATTSCAP * 1000;
    int applied = 0;

    *ry = init_ryzenadj();
    if (*ry == NULL) {
        fprintf(stderr, "init_ryzenadj failed: run as root, with the ryzen_smu module loaded (or boot with iomem=relaxed)\n");
        return -1;
    }

    if (!init_table(*ry) && !refresh_table(*ry)) {
        orig_stapm = get_stapm_limit(*ry);
        orig_fast = get_fast_limit(*ry);
        orig_slow = get_slow_limit(*ry);
        printLimits(*ry, "before capping");
    } else {
        printf("Could not read the power metrics table: the limits will not be restored at the end\n");
    }

    /* Some families reject some limits (e.g. desktop APUs may have no STAPM), so only fail if all are rejected */
    applied += !setLimit(set_stapm_limit, *ry, mw, "set_stapm_limit");
    applied += !setLimit(set_fast_limit, *ry, mw, "set_fast_limit");
    applied += !setLimit(set_slow_limit, *ry, mw, "set_slow_limit");
    if (applied == 0) {
        cleanup_ryzenadj(*ry);
        return -1;
    }

    /* The SMU clamps values outside of what the platform allows, so show what was actually applied */
    printLimits(*ry, "after capping");
    return 0;
}

void destroyRyzenadj(ryzen_access ry) {
    if (orig_stapm > 0)
        setLimit(set_stapm_limit, ry, (uint32_t)(orig_stapm * 1000), "set_stapm_limit");
    if (orig_fast > 0)
        setLimit(set_fast_limit, ry, (uint32_t)(orig_fast * 1000), "set_fast_limit");
    if (orig_slow > 0)
        setLimit(set_slow_limit, ry, (uint32_t)(orig_slow * 1000), "set_slow_limit");
    printLimits(ry, "restored");
    cleanup_ryzenadj(ry);
}
#else
/* Limits found before capping (desktop: a single package with a single die) */
static raplcap_limit orig_long, orig_short;
static int orig_saved = 0, orig_enabled = -1;

/* Like printLimits on AMD: read the limits back from the hardware */
static void printRaplLimits(raplcap *rc, const char *when) {
    raplcap_limit l, s;
    if (raplcap_pd_get_limits(rc, 0, 0, RAPLCAP_ZONE_PACKAGE, &l, &s))
        return;
    printf("Limits %s: long %.1f W, short %.1f W (%s)\n", when, l.watts, s.watts,
           raplcap_pd_is_zone_enabled(rc, 0, 0, RAPLCAP_ZONE_PACKAGE) == 1 ? "enabled" : "disabled");
}

void restoreRapl(raplcap *rc){
    if (orig_saved)
    {
        orig_long.seconds = 0;
        orig_short.seconds = 0;
        if (raplcap_pd_set_limits(rc, 0, 0, RAPLCAP_ZONE_PACKAGE, &orig_long, &orig_short))
            perror("raplcap_pd_set_limits (restore)");
    }
    if (orig_enabled >= 0 && raplcap_pd_set_zone_enabled(rc, 0, 0, RAPLCAP_ZONE_PACKAGE, orig_enabled))
        perror("raplcap_pd_set_zone_enabled (restore)");
    printRaplLimits(rc, "restored");
}

int initializeRapl(raplcap *rc){
    printf("Running with CAP\n");
    raplcap_limit rl_short, rl_long;
    uint32_t q, j, n, d;

    // initialize
    if (raplcap_init(rc))
    {// Signal handler for SIGALRM
        perror("raplcap_init");
        return -1;
    }

    // get the number of RAPL packages
    n = raplcap_get_num_packages(NULL);
    if (n == 0)
    {
        perror("raplcap_get_num_packages");
        return -1;
    }

    // assuming each package has the same number of die, only querying for package=0
    d = raplcap_get_num_die(rc, 0);
    if (d == 0)
    {
        perror("raplcap_get_num_die");
        raplcap_destroy(rc);
        return -1;
    }

    if (n != 1 || d != 1)
    {
        fprintf(stderr, "Only 1 package with 1 die is supported (found %u packages, %u dies)\n", n, d);
        raplcap_destroy(rc);
        return -1;
    }

    /* Save the original limits to put them back */
    orig_enabled = raplcap_pd_is_zone_enabled(rc, 0, 0, RAPLCAP_ZONE_PACKAGE);
    orig_saved = !raplcap_pd_get_limits(rc, 0, 0, RAPLCAP_ZONE_PACKAGE, &orig_long, &orig_short);
    if (orig_saved)
        printRaplLimits(rc, "before capping");
    else
        printf("Could not read the limits: they will not be restored at the end\n");

    // for each package die, set a power cap of same limit for both long and short
    // a time window of 0 leaves the time window unchanged
    rl_long.watts = WHATTSCAP;
    rl_long.seconds = 0;
    rl_short.watts = WHATTSCAP;
    rl_short.seconds = 0;

    for (q = 0; q < n; q++)
    {
        for (j = 0; j < d; j++)
        {
            if (raplcap_pd_set_limits(rc, q, j, RAPLCAP_ZONE_PACKAGE, &rl_long, &rl_short))
            {
                perror("raplcap_pd_set_limits");
                restoreRapl(rc);
                raplcap_destroy(rc);
                return -1;
            }
        }
    }

    // for each package die, enable the power caps
    // this could be done before setting caps, at the risk of enabling unknown power cap values first
    for (q = 0; q < n; q++)
    {
        for (j = 0; j < d; j++)
        {
            if (raplcap_pd_set_zone_enabled(rc, q, j, RAPLCAP_ZONE_PACKAGE, 1))
            {
                perror("raplcap_pd_set_zone_enabled");
                /* If the zone was already enabled the cap is in force anyway */
                if (orig_enabled != 1)
                {
                    restoreRapl(rc);
                    raplcap_destroy(rc);
                    return -1;
                }
            }
        }
    }

    printRaplLimits(rc, "after capping");
    return 0;
}
#endif

// Function to measure memory usage of a command and return the result
int runCommand(const char *command) {
    char cmd[1024];
    int mem;
    sprintf(cmd, "{ /usr/bin/time -v %s > /dev/null; } 2>&1 | grep 'Maximum resident' | sed 's/[^0-9]\\+\\([0-9]\\+\\).*/\\1/'", command);
    
    FILE *fp2 = popen(cmd, "r");
    if (fp2 == NULL) {
        fprintf(stderr, "Error running command\n");
        exit(-1);
    }

    char buf[1024];
    while (fgets(buf, sizeof(buf), fp2) != NULL) {
    }

    int status = pclose(fp2);
    mem = atoi(buf);
    return mem;
}

void* programWorks(void *c){
    char* command = (char*) c;
    FILE *fp2 = popen(command, "r");
    if (fp2 == NULL) {
        fprintf(stderr, "Error running program verification\n");
        return 0;
    }
    
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp2) != NULL) {
    }
    int status = pclose(fp2);

    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0) {
            printf("Command executed successfully\n");
            threadData.returnValue = 1;
        } else {
            printf("Command exited with an error status: %d\n", exit_status);
            threadData.returnValue = 0;
        }
    } else {
        printf("Command did not exit normally\n");
        threadData.returnValue = 0;
    }
    pthread_exit(NULL);
};

void writeErrorMessage(const char *language, const char *program, const char *errorMsg){
    FILE *fp;
    fp = fopen(MEASUREMENTS_FILE, "w");
    if (fp == NULL) {
        perror("Error opening measurements file");
        exit(-1);
    }

    fprintf(fp, "Error Language, Program, PowerLimit, Package, Core(s), GPU, DRAM, Time (ms), CacheReferences, CacheMisses, Temperature, Memory\n");
    fprintf(fp, "%s, %s, %d, %s, %s, %s, %s, %s, %s, %s, %s\n", language, program, WHATTSCAP,
                        errorMsg,errorMsg,errorMsg,errorMsg,errorMsg,errorMsg,errorMsg, errorMsg);
    fclose(fp);
}

void runTesting(int ntimes, int core, const char *command, const char *language, const char *program){
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        printf("unnespected behaviour\n");
        return;
    }

    ts.tv_sec += TIME_OUT_LIMIT;

    // Create a thread to execute the function
    pthread_t thread;
    pthread_create(&thread, NULL, programWorks, (void *)command);
    
    // Join the thread to ensure its completion and get the return value
    int state = pthread_timedjoin_np(thread, NULL, &ts);

    if (state!=0) {
        // Handle timeout: Function execution interrupted
        writeErrorMessage(language,program,"TimeOut");
        printf("Timeout: Function execution interrupted after %d seconds\n", TIME_OUT_LIMIT);
    } else {
        // Function executed within the specified timeout
        if(threadData.returnValue==1){ //Program works
            performMeasurements(command,language,program,ntimes,core);
        }
        else{
            writeErrorMessage(language,program,"ProgramError");
        }
    }
}

struct MeasureArgs {
    FILE *fp;
    int core;
    const char *command;
};

void* measure(void* arguments){
    struct MeasureArgs* args = (struct MeasureArgs*) arguments;
    clock_t begin, end;
    double time_spent;
    struct timeval tvb, tva;
    char str_temp[20];

    rapl_before(args->fp, args->core);
        
    begin = clock();
    gettimeofday(&tvb, 0);
        
    int mem = runCommand(args->command);
        
    end = clock();
    gettimeofday(&tva, 0);
    time_spent = ((tva.tv_sec - tvb.tv_sec) * 1000000 + tva.tv_usec - tvb.tv_usec) / 1000;
        
    rapl_after(args->fp, args->core);
    sprintf(str_temp, "%.1f", getTemperature());
    /* No value for user_time, system time, cache_references and cache_misses*/
    fprintf(args->fp, "%G, , , , %s, %d, %d\n", time_spent, str_temp, mem, USE_PERF);
    fflush(args->fp);
}

double read_perf_measurement (FILE *fp) {
  char line[MAX_STRING_LENGTH];
  double value;
  int d;
  d = fscanf(fp, "%s[^\n]", line);
  /* Sometimes perf does not report a value for system_time */
  /* In our setting, data about system_time should be at the last line of the file */
  /* When there is no data, we provide zero as the default value */
  /* "<not counted>" and "<not supported>" are not numbers: also use zero */
  if (d == EOF || sscanf(line, "%lf", &value) != 1) {
      value = 0;
  }
  return value;
}

void cond_read_perf(FILE* fp, bool cond, FILE* from){
  if(cond)
    fprintf(fp, " %.2lf,", read_perf_measurement(from));
  else
    fprintf(fp, "0,");
}

void build_energy_cmd(char* perf_cmd, struct event_candidates ec){
    if(ec.energy_psys) strcat(perf_cmd, "power/energy-psys/,");
    if(ec.energy_pkg) strcat(perf_cmd, "power/energy-pkg/,");
    if(ec.amd_energy_core) strcat(perf_cmd, "power_core/energy-core/,");
    if(ec.energy_cores) strcat(perf_cmd, "power/energy-cores/,");
    if(ec.energy_gpu) strcat(perf_cmd, "power/energy-gpu/,");
    if(ec.energy_ram) strcat(perf_cmd, "power/energy-ram/,");
}

void perform_perf_measurement (struct MeasureArgs* args) {
    const char *tmp_file = "measurements-perf-tmp.csv";
    struct event_candidates ec = fetch_energy_events();
    char perf_cmd[MAX_STRING_LENGTH];
    char energy_events[MAX_STRING_LENGTH];
    energy_events[0] = '\0';
    /* AMD has no PSys, GPU or DRAM counters. core counter is in the "power_core" (requires Linux >= 6.13) */
    /* const char *energy_events = cpu_vendor == CPU_VENDOR_AMD
        ? "power/energy-pkg/,power_core/energy-core/"
        : "power/energy-psys/,power/energy-pkg/,power/energy-cores/,power/energy-gpu/,power/energy-ram/"; */

    build_energy_cmd(energy_events, ec);
    const char *time_events = "duration_time,user_time,system_time"; 
    const char *cache_event = "cache-references,cache-misses";
    const char *all_cpus = "--all-cpus"; /* this flag is necessary when getting energy and time metrics */
    /* to use '.' when printing floating values (we will read them back later) */
    const char* locale_cmd = "export LANG=en_US.UTF-8";  
    char full_cmd[MAX_STRING_LENGTH];

    sprintf(perf_cmd, "sudo perf stat -x ';' -e %s%s,%s %s %s > /dev/null 2> %s", energy_events, time_events, cache_event, all_cpus, args->command, tmp_file);
    
    sprintf(full_cmd, "%s  &&  %s", locale_cmd, perf_cmd);
   
    printf("Going to execute 1: %s\n", perf_cmd);
    printf("Going to execute 2: %s\n", full_cmd);
   
    system(full_cmd);
    FILE *fp = fopen(tmp_file, "r");
    if (fp == NULL) {
        perror("Error opening temporary measurement file");
        exit(-1);
    }

    cond_read_perf(args->fp, ec.energy_psys, fp);
    cond_read_perf(args->fp, ec.energy_pkg, fp);
    if(cpu_vendor == CPU_VENDOR_AMD)
       cond_read_perf(args->fp, ec.amd_energy_core, fp);
    else
       cond_read_perf(args->fp, ec.energy_cores, fp);
    cond_read_perf(args->fp, ec.energy_gpu, fp);
    cond_read_perf(args->fp, ec.energy_ram, fp);


    /* Time (ms): duration_time gives time in ns */
    fprintf(args->fp, " %ld,", (long int)read_perf_measurement(fp) / 1000000);
    
    /* User Time (ms): user_time_time gives time in ns */
    fprintf(args->fp, " %ld,", (long int)read_perf_measurement(fp) / 1000000);

    /* System Time (ms): system_time_time gives time in ns */
    fprintf(args->fp, " %ld,", (long int)read_perf_measurement(fp) / 1000000);

    /* Cache References */
    fprintf(args->fp, " %ld,", (long int)read_perf_measurement(fp));

    /* Cache Misses */
    fprintf(args->fp, " %ld,", (long int)read_perf_measurement(fp));

    /* Temperate, Memory, Usage of perf: Not providing an actual value for Memory */
    fprintf(args->fp, " %.1f, , %d\n", getTemperature(), USE_PERF);
    
    fclose(fp);
}


// Function to perform the measurements and write to the measurements file
void performMeasurements(const char *command, const char *language, const char *program, int ntimes, int core) {
    struct timespec ts;

    FILE *fp;
    float temperature;

    if (!USE_PERF) /* MSR units are only used by rapl_before/rapl_after */
        rapl_init(core);

    fp = fopen(MEASUREMENTS_FILE, "w");
    if (fp == NULL) {
        perror("Error opening measurements file");
        exit(-1);
    }

    fprintf(fp, "Language, Program, PowerLimit, PSys, Package, Core(s), GPU, DRAM, Time (ms), User Time, Sys Time, Cache References, Cache Misses, Temperature, Memory, Perf\n");

    for (int i = 0; i < ntimes; i++) {
        fprintf(fp, "%s, %s, %d, ", language, program, WHATTSCAP);
        temperature = getTemperature();

        while (temperature > TEMPERATURETHRESHOLD + VARIANCE) {
            printf("Sleeping\n");
            sleep(1);
            temperature = getTemperature();
        }

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1){
            printf("unnespected behaviour\n");
            return;
        }

        ts.tv_sec += TIME_OUT_LIMIT;

        pthread_t thread;
        struct MeasureArgs* myArgs = (struct MeasureArgs*)malloc(sizeof(struct MeasureArgs));
        myArgs->command=command;
        myArgs->core=core;
        myArgs->fp=fp;
        if (USE_PERF) {
            perform_perf_measurement(myArgs);
        } else {
            fprintf(fp, " ,"); /* No value for PSys */
            pthread_create(&thread, NULL, measure, (void *)myArgs);
            
            // Join the thread to ensure its completion and get the return value
            int state = pthread_timedjoin_np(thread, NULL, &ts);
            if (state!=0) {
                // Handle timeout: Function execution interrupted
                writeErrorMessage(language,program,"TimeOut");
                printf("Timeout: Function execution interrupted after %d seconds\n", TIME_OUT_LIMIT);
                return;
            }
        }
        
        
        
    }
    fclose(fp);
}



int main(int argc, char **argv) {
    char command[MAX_COMMAND_LENGTH];
    char language[MAX_STRING_LENGTH] = "";
    char program[MAX_STRING_LENGTH] = "";
    int ntimes = 3;
    int core = 0;
#ifdef USE_RYZENADJ
    ryzen_access ry = NULL;
#else
    raplcap rc;
#endif

    if (argc < 5) {
        printf("Usage: %s <command> <language> <program> <ntimes>\n", argv[0]);
        return -1;
    }

    strcpy(command, argv[1]);
    strcpy(language, argv[2]);
    strcpy(program, argv[3]);
    ntimes = atoi(argv[4]);

    printf("\n\n Command: %s | Language: %s | Program: %s | Ntimes %d \n\n", command, language, program, ntimes);

    fflush(stdout);

    detect_cpu();

    /* AMD only support perf */
    if (cpu_vendor == CPU_VENDOR_AMD && !USE_PERF) {
        printf("USE_PERF=0 is not supported on AMD CPUs, use USE_PERF=1\n");
        return -1;
    }

    if (WHATTSCAP != -1) {
#ifdef USE_RYZENADJ
        if (cpu_vendor != CPU_VENDOR_AMD) {
            printf("ryzenadj only supports AMD CPUs: rebuild with `make -B CAP_BACKEND=raplcap`\n");
            exit(-1);
        }
        if (initializeRyzenadj(&ry)) {
            writeErrorMessage(language, program, "CapError");
            return -1;
        }
#else
        /* raplcap fails on AMD */
        if (cpu_vendor == CPU_VENDOR_AMD) {
            printf("Power capping on AMD needs ryzenadj: rebuild with `make -B CAP_BACKEND=ryzenadj`\n");
            exit(-1);
        }
        if(initializeRapl(&rc)) {
            writeErrorMessage(language, program, "CapError");
            return -1;
        }
#endif
    }
    runTesting(ntimes,core, command, language, program);
    printf("\n\n END of PARAMETRIZED PROGRAM: \n");

    if (WHATTSCAP != -1) {
#ifdef USE_RYZENADJ
        destroyRyzenadj(ry);
#else
        restoreRapl(&rc);
        if (raplcap_destroy(&rc)) {
            printf("Error destroying CAP\n");
            perror("raplcap_destroy");
        } else {
            printf("Successfully destroyed CAP\n");
        }
#endif
    }
    return 0;
    
}

