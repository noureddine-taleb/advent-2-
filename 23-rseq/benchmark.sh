#!/bin/bash

rm data

for mode in rseq-atomic rseq regular lock getcpu-atomic ; do
    for threads in `seq 1 32`; do
        ./rseq $threads $mode 8 >> data
    done
done

echo "Run: ./plot.py data"
