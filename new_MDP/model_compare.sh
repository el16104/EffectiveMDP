#!/bin/bash
for i in {1..20}
do
    for j in 100 250 500 750 1000 2500 5000 7500 10000 12500 15000 17500 100000
    do
        ./run_model.sh infinite $j $i
        ./run_model.sh naive $j $i
        ./run_model.sh inplace $j $i
        ./run_model.sh root $j $i
    done
done