#include "rapl.h"


int cpu_model;
int cpu_vendor;
int core=0;

uint32_t package_before;
uint32_t pp0_before;
uint32_t pp1_before=0;
uint32_t dram_before=0;

double power_units,energy_units,time_units;

static double energy_delta(uint32_t before, uint32_t after) {
  return (double)(uint32_t)(after-before)*energy_units;
}

int open_msr(int core) {

  char msr_filename[BUFSIZ];
  int fd;

  sprintf(msr_filename, "/dev/cpu/%d/msr", core);
  fd = open(msr_filename, O_RDONLY);
  if ( fd < 0 ) {
    if ( errno == ENXIO ) {
      fprintf(stderr, "rdmsr: No CPU %d\n", core);
      exit(2);
    } else if ( errno == EIO ) {
      fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n", core);
      exit(3);
    } else {
      perror("rdmsr:open");
      fprintf(stderr,"Trying to open %s\n",msr_filename);
      exit(127);
    }
  }

  return fd;
}

long long read_msr(int fd, uint32_t which) {

  uint64_t data;

  if ( pread(fd, &data, sizeof data, (off_t)which) != sizeof data ) {
    perror("rdmsr:pread");
    exit(127);
  }

  return (long long)data;
}

#define CPU_SANDYBRIDGE   42
#define CPU_SANDYBRIDGE_EP  45
#define CPU_IVYBRIDGE   58
#define CPU_IVYBRIDGE_EP  62
#define CPU_HASWELL   60
#define CPU_HASWELL2   69
#define CPU_HASWELL3   70
#define CPU_HASWELL_EP   63
#define CPU_SKYLAKE1   78
#define CPU_SKYLAKE2   94
#define CPU_BROADWELL  77
#define CPU_BROADWELL2  79
#define CPU_KABYLAKE 158

int detect_cpu(void) {

	FILE *fff;

	int family,model=-1;
	char buffer[BUFSIZ],*result;
	char vendor[BUFSIZ];

	fff=fopen("/proc/cpuinfo","r");
	if (fff==NULL) return -1;

	while(1) {
		result=fgets(buffer,BUFSIZ,fff);
		if (result==NULL) break;

		if (!strncmp(result,"vendor_id",8)) {
			sscanf(result,"%*s%*s%s",vendor);

			if (!strncmp(vendor,"GenuineIntel",12)) cpu_vendor=CPU_VENDOR_INTEL;
			else if (!strncmp(vendor,"AuthenticAMD",12)) {
				cpu_vendor=CPU_VENDOR_AMD;
			} else {
				printf("%s not an Intel chip\n",vendor);
				return -1;
			}
		}

		if (!strncmp(result,"cpu family",10)) {
			sscanf(result,"%*s%*s%*s%d",&family);
			if ((cpu_vendor==CPU_VENDOR_INTEL && family!=6) || (cpu_vendor==CPU_VENDOR_AMD && family<AMD_MIN_FAMILY)) {
				printf("Wrong CPU family %d\n",family);
				return -1;
			}
		}

		if (!strncmp(result,"model",5)) {
			sscanf(result,"%*s%*s%d",&model);
		}

	}

	fclose(fff);
/**
	switch(model) {
		case CPU_SANDYBRIDGE:
			printf("Found Sandybridge CPU\n");
			break;
		case CPU_SANDYBRIDGE_EP:
			printf("Found Sandybridge-EP CPU\n");
			break;
		case CPU_IVYBRIDGE:
			printf("Found Ivybridge CPU\n");
			break;
		case CPU_IVYBRIDGE_EP:
			printf("Found Ivybridge-EP CPU\n");
			break;
    case CPU_HASWELL:
      printf("Found Haswell CPU\n");
      break;
    case CPU_HASWELL2:
      printf("Found Haswell2 CPU\n");
      break;
    case CPU_HASWELL3:
      printf("Found Haswell3 CPU\n");
      break;
    case CPU_HASWELL_EP:
      printf("Found Haswell_EP CPU\n");
      break;
    case CPU_SKYLAKE1:
      printf("Found SKYLAKE1 CPU\n");
      break;
    case CPU_SKYLAKE2:
      printf("Found SKYLAKE2 CPU\n");
      break;
    case CPU_BROADWELL:
      printf("Found BROADWELL CPU\n");
      break;
    case CPU_BROADWELL2:
      printf("Found BROADWELL2 CPU\n");
      break;
    case CPU_KABYLAKE:
      printf("Found KABYLAKE CPU\n");
      break;
		default:	printf("Unsupported model %d\n",model);
				//model=-1;
				break;
	}
**/
	return model;
}






static int amd_pkg_fd=-1;
static int amd_core_fd[AMD_MAX_CORES];
static int amd_ncores=0;
static uint32_t amd_pkg_before, amd_core_before[AMD_MAX_CORES];

static int read_cpu_sysfs(int cpu, const char *file, int *value) {
  char path[BUFSIZ];
  FILE *f;
  int ok;

  snprintf(path, sizeof path, "/sys/devices/system/cpu/cpu%d/%s", cpu, file);
  f=fopen(path,"r");
  if (f==NULL) return -1;
  ok=fscanf(f,"%d",value)==1;
  fclose(f);
  return ok ? 0 : -1;
}

static int amd_online_cores(int *cpus, int max) {
  int pkgs[AMD_MAX_CORES], cores[AMD_MAX_CORES];
  int n=0, ncpus=(int)sysconf(_SC_NPROCESSORS_CONF);

  for (int cpu=0; cpu<ncpus && n<max; cpu++) {
    int online=1, pkg, core_id, seen=0;

    read_cpu_sysfs(cpu, "online", &online);   /* cpu0 has no "online" file */
    if (!online) continue;
    if (read_cpu_sysfs(cpu, "topology/physical_package_id", &pkg) ||
        read_cpu_sysfs(cpu, "topology/core_id", &core_id)) continue;
    for (int i=0; i<n; i++)
      if (pkgs[i]==pkg && cores[i]==core_id) seen=1;   /* SMT sibling: shares the core counter */
    if (seen) continue;
    pkgs[n]=pkg;
    cores[n]=core_id;
    cpus[n++]=cpu;
  }
  return n;
}

static int amd_rapl_init(int core) {
  int cpus[AMD_MAX_CORES];
  long long result;

  for (int i=0; i<amd_ncores; i++) close(amd_core_fd[i]);
  if (amd_pkg_fd>=0) close(amd_pkg_fd);

  amd_ncores=amd_online_cores(cpus, AMD_MAX_CORES);
  if (amd_ncores==0) {
    fprintf(stderr, "No online core found to read the AMD core energy counter\n");
    return -1;
  }
  for (int i=0; i<amd_ncores; i++) amd_core_fd[i]=open_msr(cpus[i]);
  amd_pkg_fd=open_msr(core);

  result=read_msr(amd_pkg_fd,MSR_AMD_RAPL_POWER_UNIT);
  power_units=pow(0.5,(double)(result&0xf));
  energy_units=pow(0.5,(double)((result>>8)&0x1f));
  time_units=pow(0.5,(double)((result>>16)&0xf));
  return 0;
}

static void amd_rapl_before(void) {
  amd_pkg_before=(uint32_t)read_msr(amd_pkg_fd,MSR_AMD_PKG_ENERGY_STATUS);
  for (int i=0; i<amd_ncores; i++)
    amd_core_before[i]=(uint32_t)read_msr(amd_core_fd[i],MSR_AMD_CORE_ENERGY_STATUS);
}

static void amd_rapl_after(FILE *fp) {
  uint32_t pkg_after=(uint32_t)read_msr(amd_pkg_fd,MSR_AMD_PKG_ENERGY_STATUS);
  double core_energy=0.0;

  for (int i=0; i<amd_ncores; i++)
    core_energy+=energy_delta(amd_core_before[i],(uint32_t)read_msr(amd_core_fd[i],MSR_AMD_CORE_ENERGY_STATUS));

  fprintf(fp,"%.18f, ",energy_delta(amd_pkg_before,pkg_after));
  fprintf(fp,"%.18f, ",core_energy);
  fprintf(fp," , ");  // GPU: no counter on AMD
  fprintf(fp," , ");  // DRAM: no counter on AMD
}


int rapl_init(int core)
{ int fd;
  long long result;

  cpu_model=detect_cpu();
  if (cpu_model<0) {
  printf("Unsupported CPU type\n");
  return -1;
  }

  if (cpu_vendor==CPU_VENDOR_AMD)
    return amd_rapl_init(core);

  // printf("Checking core #%d\n",core);

  fd=open_msr(core);

  /* Calculate the units used */
  result=read_msr(fd,MSR_RAPL_POWER_UNIT);

  power_units=pow(0.5,(double)(result&0xf));
  energy_units=pow(0.5,(double)((result>>8)&0x1f));
  time_units=pow(0.5,(double)((result>>16)&0xf));

  /*
  printf("Power units = %.3fW\n",power_units);
  printf("Energy units = %.8fJ\n",energy_units);
  printf("Time units = %.8fs\n",time_units);
  printf("\n");
  */


  return 0;
}


void show_power_info(int core)
{ int fd;
  long long result;
  double thermal_spec_power,minimum_power,maximum_power,time_window;



 /* Show package power info */

  fd=open_msr(core);
  result=read_msr(fd,MSR_PKG_POWER_INFO);

  thermal_spec_power=power_units*(double)(result&0x7fff);
  printf("Package thermal spec: %.3fW\n",thermal_spec_power);

  minimum_power=power_units*(double)((result>>16)&0x7fff);
  printf("Package minimum power: %.3fW\n",minimum_power);

  maximum_power=power_units*(double)((result>>32)&0x7fff);
  printf("Package maximum power: %.3fW\n",maximum_power);

  time_window=time_units*(double)((result>>48)&0x7fff);
  printf("Package maximum time window: %.6fs\n",time_window);
}



void show_power_limit(int core)
{ int fd;
  long long result;


 /* Show package power limit */

  fd=open_msr(core);
  result=read_msr(fd,MSR_PKG_RAPL_POWER_LIMIT);

  printf("Package power limits are %s\n", (result >> 63) ? "locked" : "unlocked");
  double pkg_power_limit_1 = power_units*(double)((result>>0)&0x7FFF);
  double pkg_time_window_1 = time_units*(double)((result>>17)&0x007F);
  printf("Package power limit #1: %.3fW for %.6fs (%s, %s)\n", pkg_power_limit_1, pkg_time_window_1,
           (result & (1LL<<15)) ? "enabled" : "disabled",
           (result & (1LL<<16)) ? "clamped" : "not_clamped");
  double pkg_power_limit_2 = power_units*(double)((result>>32)&0x7FFF);
  double pkg_time_window_2 = time_units*(double)((result>>49)&0x007F);
  printf("Package power limit #2: %.3fW for %.6fs (%s, %s)\n", pkg_power_limit_2, pkg_time_window_2,
          (result & (1LL<<47)) ? "enabled" : "disabled",
          (result & (1LL<<48)) ? "clamped" : "not_clamped");

  printf("\n");

}




void rapl_before(FILE * fp,int core)
{ int fd;
  long long result;

  if (cpu_vendor==CPU_VENDOR_AMD) {
    amd_rapl_before();
    return;
  }

  fd=open_msr(core);
  result=read_msr(fd,MSR_PKG_ENERGY_STATUS);

  package_before=(uint32_t)result;

  /* only available on *Bridge-EP */
  if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP))
  {
    result=read_msr(fd,MSR_PKG_PERF_STATUS);
    double acc_pkg_throttled_time=(double)result*time_units;
    // fprintf(fp,"Accumulated Package Throttled Time : %.6fs\n",acc_pkg_throttled_time);
  }

  result=read_msr(fd,MSR_PP0_ENERGY_STATUS);
  pp0_before=(uint32_t)result;

  result=read_msr(fd,MSR_PP0_POLICY);
  int pp0_policy=(int)result&0x001f;
  // fprintf(fp,"PowerPlane0 (core) for core %d policy: %d\n",core,pp0_policy);

  /* only available on *Bridge-EP */
  if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP))
  {
    result=read_msr(fd,MSR_PP0_PERF_STATUS);
    double acc_pp0_throttled_time=(double)result*time_units;
    // fprintf(fp,"PowerPlane0 (core) Accumulated Throttled Time : %.6fs\n",acc_pp0_throttled_time);
  }

  /* not available on *Bridge-EP */
  if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE) ||
  (cpu_model==CPU_HASWELL)) {
     result=read_msr(fd,MSR_PP1_ENERGY_STATUS);
     pp1_before=(uint32_t)result;
     result=read_msr(fd,MSR_PP1_POLICY);
     int pp1_policy=(int)result&0x001f;
     //fprintf(fp,"PowerPlane1 (on-core GPU if avail) %d policy: %d\n",core,pp1_policy);
  }

  /* Despite documentation saying otherwise, it looks like */
  /* You can get DRAM readings on regular Haswell          */
  if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP) ||
  (cpu_model==CPU_HASWELL)) {
     result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);
     dram_before=(uint32_t)result;
  }

}


void rapl_after(FILE * fp , int core)
{ int fd;
  long long result;

  if (cpu_vendor==CPU_VENDOR_AMD) {
    amd_rapl_after(fp);
    return;
  }

  fd=open_msr(core);

  result=read_msr(fd,MSR_PKG_ENERGY_STATUS);
  fprintf(fp,"%.18f, ",energy_delta(package_before,(uint32_t)result));  // PACKAGE

  result=read_msr(fd,MSR_PP0_ENERGY_STATUS);
  fprintf(fp,"%.18f, ",energy_delta(pp0_before,(uint32_t)result));    // CORE


  /* not available on SandyBridge-EP */
  if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE) ||
  (cpu_model==CPU_HASWELL)) {
     result=read_msr(fd,MSR_PP1_ENERGY_STATUS);
     fprintf(fp,"%.18f, ",energy_delta(pp1_before,(uint32_t)result));     // GPU
  }
  else
    fprintf(fp," , ");

  if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP) ||
  (cpu_model==CPU_HASWELL)) {
     result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);
     fprintf(fp,"%.18f, ",energy_delta(dram_before,(uint32_t)result));     // DRAM
  }
  else
    fprintf(fp," , ");  

}
