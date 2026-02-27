import pandas as pd
import os
import sys
import statistics
import matplotlib.pyplot as plt
import numpy as np
#from scipy import stats
#from cliffs_delta import cliffs_delta

def show_diffs (xs):
    for x in xs:
        y = (1 - x) * 100
        print(f"{x:.2f}", end=", ")

def calc_time_diff (df1, df2):
    diffs = []
    for index, row in df1.iterrows():
      x = row['perf_time'] / row['duration_time']
      diffs.append(x)
    for index, row in df2.iterrows():
      x = row['perf_time'] / row['duration_time']
      diffs.append(x)

    return diffs

def machineflag(file):
    machine=''
    flag=''
    if 'elite' in files[0]:
        machine = 'ELITE'
    if 'think' in files[0]:
        machine = 'THINK'
    if 'O0' in files[0]:
        flag = 'O0'
    if 'O2' in files[0]:
        flag = 'O2'
    return machine, flag

def openfiles(files):
    df = list()
    for f in files:
        #print(f)
        path = os.path.join(directory, f)
        df.append(pd.read_csv(path))
    return df

def extraitempos(df):

    tempo = []
    tempo_over = []
    tempo_cfggrind = []
    for j in range(len(df)):
        if 'cfggrind' in files[j]:
            tempo_cfggrind = np.concatenate((tempo_cfggrind, df[j].loc[:,['cfgrind_time']].values), axis=None)  
        else:
            tempo = np.concatenate((tempo, df[j].loc[:,['duration_time']].values), axis=None)  
            tempo_over = np.concatenate((tempo_over, df[j].loc[:,['perf_time']].values), axis=None)  

    tempo = tempo/(10**6)   
    tempo_over = tempo_over/(10**6)
    tempo_cfggrind = tempo_cfggrind/(10**6)
    
    return tempo, tempo_over, tempo_cfggrind

def calc_wicoxon (problema, tempo, tempo_perf, tempo_cfggrind):
  print(f"----  {problema}   ----")
  statistic, pvalue = stats.wilcoxon(tempo, tempo_perf)
  print("Tempo Original x Perf: ", statistic, pvalue)
  if pvalue < 0.05:
    d, res = cliffs_delta(tempo, tempo_perf)
    print(f"Cliff {d}, {res}")
  
  statistic, pvalue = stats.wilcoxon(tempo, tempo_cfggrind)
  print("Tempo Original x CFGgrind = ", statistic, pvalue)
  if pvalue < 0.05:
    d, res = cliffs_delta(tempo, tempo_cfggrind)
    print(f"Cliff {d}, {res}")

  print()



def calc_mww (problema, tempo, tempo_perf, tempo_cfggrind):
  print(f"----  {problema}   ----")
  print("Tempo Original x Perf: ", stats.mannwhitneyu(tempo, tempo_perf))
  
  
  print("Tempo Original x CFGgrind = ", stats.mannwhitneyu(tempo, tempo_cfggrind))
  print()

arquivos = sys.argv


directory = os.path.join(".",arquivos[1])
diffsabsolutatudo =[]
diffsabsolutatudocfggrind =[]
medianaProblema = []
labels = []
vprob = []
temposmedianos = []
temposovermedianos = []
temposcfggrindmedianos =[]
for root,dirs,files in os.walk(directory):
    
    #for file in sorted(files):
    files = sorted(files)
    for i in range(0, len(files),4):
       
       df = openfiles(files[i:i+4])
        
       tempo, tempo_over, tempo_cfggrind = extraitempos(df)
       #calc_mww(files[i][:5], tempo, tempo_over, tempo_cfggrind)
       #calc_wicoxon(files[i][:5], tempo, tempo_over, tempo_cfggrind) 

       temposmedianos.append(statistics.median(tempo))
       temposovermedianos.append(statistics.median(tempo_over))
       temposcfggrindmedianos.append(statistics.median(tempo_cfggrind))

       diffsabsolutatudo = np.concatenate((diffsabsolutatudo, np.array(temposovermedianos)-np.array(temposmedianos)), axis=None)
       diffsabsolutatudocfggrind = np.concatenate((diffsabsolutatudocfggrind, np.array(temposcfggrindmedianos)-np.array(temposmedianos)), axis=None)

       labels.append(files[i][0:4])
       vprob.append(i/4)


print(f"Mediana da diferenca de tempo Total PERF = {statistics.median(diffsabsolutatudo)}")
print(f"Media da diferenca de tempo Total PERF = {statistics.mean(diffsabsolutatudo)}")
print(f"Minimo da diferenca de tempo Total PERF = {min(diffsabsolutatudo)}")
print(f"Maximo da diferenca de tempo Total PERF = {max(diffsabsolutatudo)}")

print(f"Mediana da diferenca de tempo Total CFGGRIND = {statistics.median(diffsabsolutatudocfggrind)}")
print(f"Media da diferenca de tempo Total CFGGRIND = {statistics.mean(diffsabsolutatudocfggrind)}")
print(f"Minimo da diferenca de tempo Total CFGGRIND = {min(diffsabsolutatudocfggrind)}")
print(f"Maximo da diferenca de tempo Total CFGGRIND = {max(diffsabsolutatudocfggrind)}")

print("Tempos mediana", temposmedianos)
print("Tempos mediana perf", temposovermedianos)
print("Tempos mediana cfggrind", temposcfggrindmedianos)


#sem ser normalizado, mostrando os tempos

machine, flag = machineflag(files[0])
fig, ax = plt.subplots()
width = 0.75
#ax.bar(vprob, temposmedianos, width, label='Median wall-clock time', bottom=np.zeros(len(vprob)), color='blue')
ax.bar(vprob, temposmedianos, width, label='Median wall-clock time', bottom=np.zeros(len(vprob)))
ax.bar(vprob, np.array(temposovermedianos)-np.array(temposmedianos), width, label='Median overhead Perf', bottom=temposmedianos)
ax.bar(vprob, np.array(temposcfggrindmedianos)-np.array(temposmedianos), width, label='Median overhead CFGgrind', bottom=temposovermedianos)
ax.legend(loc="upper left")

plt.xlabel('Problem')
ax.set_xticks(vprob, labels, rotation='vertical') 
plt.title('Median overhead by problem (' + machine + ' machine -'+ flag + ' flag)')
plt.ylabel('Median wall-clock time (ms)')

plt.show()
