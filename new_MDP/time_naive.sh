#!/bin/bash
for i in 557
do
    for j in 100 250 500 750 1000 2500 5000 7500 10000 25000 50000 75000 100000
    do
        ./run_model.exe naive $j $i
    done
done