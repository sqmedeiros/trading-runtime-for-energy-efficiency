# trading-runtime-for-energy-efficiency

Repository to replicate/reproduce the experiment described in paper **Trading Runtime for Energy Efficiency: Leveraging Power Caps to Save Energy across Programming Languages**

DOI: https://doi.org/10.6084/m9.figshare.27087901.v1

Software: https://doi.org/10.6084/m9.figshare.27087901.v1

We focus only on a few languages: C++, Python and Java.

### Requirements
- Debian-based Linux distributions (needs to work with RAPLCap)
- Intel processor
- Non containerized environment

### Directory Structure
- **`benchmarks/`**  
  Contains benchmark tests (see benchmarks/README.md for more details), including:
  - Dacapo
  - Nofib
  - PyPerformance

- **`inputs/`**  
  Required input files for specific problem executions.

- **`Languages/`**  
  Folder with all the languages and problems used for evaluations.

- **`NoteBooks/`**  
  Jupyter notebooks with graphs and calculations used in the paper.

- **`RAPL/`**  
  C program for measuring and limiting CPU power.


### Required Libraries
1. RAPL
2. lm-sensors
3. Powercap
4. Raplcap

These libraries can be installed with the following command:

```bash
sudo sh raplLibrariesSetup.sh
```

### Setup
1. To install all the required language compilers, interpreters and libraries, execute the script:

```bash
sudo sh languagesSetup.sh
```

Note: This setup was not fully tested and my require manual interaction (such as accepting permitions).

2. Generate the input files:

```bash
sudo sh gen-input.sh
```

3. Execute the script to generate the CSV file (this script iterates all the Languages and all of the programs):

Edit ``measure.sh`` to use the *perf* tool or not. Set variable `USE_PERF` to zero in order to replicate the original work that does not use *perf*.

```bash
sh measure.sh
```

Note: You might need to update some of the compilers path since we did not change some of the default installation paths from our machine (the paths are defined on config.env).

### Meaning of the CSV file columns

|      Column      |                        Meaning                                                                     |
|:----------------:|:--------------------------------------------------------------------------------------------------:|
|    **Language**  | Programming language of the sorting algorithm                                                      |
|    **Program**   | Name of the sorting algorithm                                                                      |
|  **PowerLimit**  | Power cap of the cores (in Watts)                                                                  |
|    **Package**   | Energy consumption of the entire socket - all cores consumption, GPU, and external core components |
|     **Core**     | Energy consumption by all cores and caches                                                         |
|     **GPU**      | Energy consumption by the GPU                                                                      |
|     **DRAM**     | Energy consumption by the RAM                                                                      |
|     **Time**     | Algorithm's execution time (in ms)                                                                 |
| **Temperature**  | Mean temperature in all cores (in Celsius degrees)                                                 |
|    **Memory**    | Total physical memory assigned to the algorithm execution (in KBytes)                              |
|     **Perf**     | Flag indicating if the perf tool was used to get energy measurements                                                      |

