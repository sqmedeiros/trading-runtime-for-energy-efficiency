#!/bin/bash

mkdir -p inputs

echo "Generating input for k-nucleotide benchmark"
python3 Utils/gen_input.py 25000000 > ./inputs/knucleotide-input25000000.txt

echo "Generating input for reverse-complement benchmark"
python3 Utils/gen_input.py 25000000 > ./inputs/revcomp-input25000000.txt

echo "Generating input for regex-redux benchmark"
python3 Utils/gen_input.py 5000000 > ./inputs/regexredux-input5000000.txt
