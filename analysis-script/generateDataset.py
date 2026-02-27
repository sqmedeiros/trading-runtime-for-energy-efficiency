# -*- coding: utf-8 -*-
"""
Created on Sat May 13 09:26:47 2023

@author: Marcelo

Lists all files in a directory and creat a dataset for weka with all the dataset
The atributes of the dataset will be the given by the metrics in the first line of the csv of each file (they must be equal in every file)
cpu-cycles,instructions,cache-references,cache-misses,branch-instructions,branch-misses,page-faults,branch-loads,branch-load-misses,L1-dcache-loads,L1-dcache-load-misses,L1-dcache-stores,L1-icache-load-misses,LLC-loads,LLC-load-misses,LLC-stores,LLC-store-misses,dTLB-loads,dTLB-load-misses,dTLB-stores,dTLB-store-misses,iTLB-loads,iTLB-load-misses,user_time,system_time
The class will be the number of the problem
1071, 1082,...
"""

from signal import signal
import sys
import pandas
import matplotlib.pyplot as plt
import numpy as np
import os
from scipy import stats
import platform
import glob


        
def listaarquivos(arquivos):
    lista = glob.glob(arquivos)
    return lista


def pegaatributos(file):
    with open(file, 'r+') as f:
        firstLine = f.readline() 
        firstLine = firstLine.rstrip('\n')
        f.close()
        return firstLine.split(',')


def cabecalho(lista):
    texto = "@relation problema\n"
    #le a primeira linha do primeiro arquivo pra pegar o nome dos atributos
    atributos = pegaatributos(lista[0])
    for i in range(1,len(atributos)):
        texto += "@attribute '" + atributos[i] + "' real\n"

    #descobre quantos programas tem no diretorio
    listanumeroprogramas = ''
    for i in range(len(lista)):
        listanumeroprogramas += str(i) 
        if i<len(lista)-1:
            listanumeroprogramas += ','
    texto += "@attribute 'class' {" + listanumeroprogramas + "}\n"
    texto += "@data\n"
    return texto
#@attribute 'preg' real



def escrevearquivo(texto, arquivodataset):
    with open(arquivodataset,"w") as f:
        f.write(texto) # write the data back
        f.truncate() # set the file size to the current size
        f.close()

def trata(l):
    l = l.rstrip('\n')
    l = l.replace("<not counted>", "?")
    l = l.replace("<not supported>", "?")
    i = l.find(".exe")
    return l[i+5:]

def pegavalores(file, ind):
    texto = ""
    print("abrindo ", file)
    with open(file, 'r+') as f:
        firstLine = f.readline() # ignora a primeira linha
        for linha in f:
            linha = trata(linha)
            texto += linha + "," + str(ind) + "\n"
        f.close()
    return texto


########   main   ##################
arquivodataset = "dataset.arff"
sistemaoperacional = platform.system()
print('Executing on ', sistemaoperacional)

arquivos = sys.argv

lista = listaarquivos(arquivos[1])

#texto do cabecalho do arquivo dataset.arff
texto = cabecalho(lista)

cont = 0
for i in lista:
     texto += pegavalores(i,cont)
     cont +=1
escrevearquivo(texto, arquivodataset)