#!/bin/bash
for i in 79 885 987 297 781	409	62 675 557 17 485 102 894 343 415 999 747 159 891 332
do
    for j in 1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288
    do
        #./run_model.sh naive $j $i 1 >> SINGLE_VARREW_NEW_NAIVE.out &&
        #./run_model.sh infinite $j $i 0.1 >> VARREW_NEW_INF_1.out &&
        #./run_model.sh infinite $j $i 0.3 >> VARREW_NEW_INF_3.out &&
        ./run_model.sh infinite $j $i 0.99 >> SINGLE_VARREW_NEW_INF_9.out &&
        ./run_model.sh infinite $j $i 0.1 >> SINGLE_VARREW_NEW_INF_1.out &&
        ./run_model.sh infinite $j $i 0.3 >> SINGLE_VARREW_NEW_INF_3.out
        #./run_model.sh infinitem $j $i 0.99 >> SINGLE_VARREW_NEW_INFM.out
    done
done
