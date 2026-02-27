import pandas as pd
import os
import sys
import statistics
import matplotlib.pyplot as plt
import numpy as np


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
      print(diffs)

    return diffs


def openfiles(files,i):
    file = files[i]
    print(file)
    path = os.path.join(directory, file)
    df1 = pd.read_csv(path)
    file = files[i+1]
    print(file)
    path = os.path.join(directory, file)
    df2 = pd.read_csv(path)


    return df1,df2, file

arquivos = sys.argv


directory = os.path.join(".",arquivos[1])
diffstudo = []
diffsabsolutatudo =[]
medianaProblema = []
labels = []
vprob = []
temposmedianos = []
temposovermedianos = []
for root,dirs,files in os.walk(directory):
    #for file in sorted(files):
    for i in range(0, len(files),2):
       df1, df2, file = openfiles(files,i)
       diffs = calc_time_diff(df1, df2)
       diffstudo = diffstudo + diffs
       #show_diffs(diffs)
       print()
       print(f"Mediana = {statistics.median(diffs)}")
       medianaProblema.append(statistics.median(diffs))
       labels.append(file[0:4])
       vprob.append(i/2)
       
       
       tempo = np.concatenate((df1.loc[:,['duration_time']].values, df2.loc[:,['duration_time']].values), axis=None)  
       tempo = tempo/(10**6)
       temposmedianos.append(statistics.median(tempo))
       tempo_over = np.concatenate((df1.loc[:,['perf_time']].values, df2.loc[:,['perf_time']].values), axis=None)  
       tempo_over = tempo_over/(10**6)
       temposovermedianos.append(statistics.median(tempo_over))
       diffsabsolutatudo = np.concatenate((diffsabsolutatudo, np.array(temposovermedianos)-np.array(temposmedianos)), axis=None)



# print(temposmedianos)
# print(temposovermedianos)
# print(np.array(temposovermedianos)-np.array(temposmedianos))

print(f"Mediana Total = {statistics.median(diffstudo)}")
print(f"Maximo Total = {max(diffstudo)}")
print(f"Minimo Total = {min(diffstudo)}")
print(f"Media Total = {statistics.mean(diffstudo)}")
print(f"Mediana da diferenca de tempo Total = {statistics.median(diffsabsolutatudo)}")
print(f"Media da diferenca de tempo Total = {statistics.mean(diffsabsolutatudo)}")
print(f"Minimo da diferenca de tempo Total = {min(diffsabsolutatudo)}")
print(f"Maximo da diferenca de tempo Total = {max(diffsabsolutatudo)}")



#fig, ax = plt.subplots()

#barra unica
#plt.bar(vprob,medianaProblema)

#duas barras
# width = 0.5
# ax.bar(vprob, np.ones(len(vprob)), width, label='normalzed median wall-clock time', bottom=np.zeros(len(vprob)))
# ax.bar(vprob, np.array(medianaProblema)-1, width, label='overhead', bottom=np.ones(len(vprob)))
# ax.legend(loc="upper left")

# plt.xlabel('Problem')
# ax.set_xticks(vprob, labels, rotation='vertical') 
# plt.title('Median Overhead by problem (' + file[-17:-12] + ' machine -'+ arquivos[1][-3:-1] + ' flag)')
# plt.ylabel('Median Overhead (percentage)')

#sem ser normalizado, mostrando os tempos

fig, ax = plt.subplots()
width = 0.75
ax.bar(vprob, temposmedianos, width, label='Median wall-clock time', bottom=np.zeros(len(vprob)))
ax.bar(vprob, np.array(temposovermedianos)-np.array(temposmedianos), width, label='Median overhead', bottom=temposmedianos)
ax.legend(loc="upper left")

plt.xlabel('Problem')
ax.set_xticks(vprob, labels, rotation='vertical') 
plt.title('Median Overhead by problem (' + file[-14:-9] + ' machine -'+ arquivos[1][-3:-1] + ' flag)')
plt.ylabel('Median wall-clock time (ms)')

plt.show()
