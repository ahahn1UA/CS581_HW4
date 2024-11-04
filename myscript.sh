#!/bin/bash
source /apps/profiles/modules_asax.sh.dyn
module load openmpi/4.1.4-gcc11

mpicc -g -Wall -std=c99 -o hw4 hw4.c
mpirun -n 1 hw4 5000 5000 1 outputs/output.txt
mpirun -n 1 hw4 5000 5000 1 outputs/output.txt
mpirun -n 1 hw4 5000 5000 1 outputs/output.txt

mpirun -n 2 hw4 5000 5000 2 outputs/output.txt
mpirun -n 2 hw4 5000 5000 2 outputs/output.txt
mpirun -n 2 hw4 5000 5000 2 outputs/output.txt

mpirun -n 4 hw4 5000 5000 4 outputs/output.txt
mpirun -n 4 hw4 5000 5000 4 outputs/output.txt
mpirun -n 4 hw4 5000 5000 4 outputs/output.txt

mpirun -n 8 hw4 5000 5000 8 outputs/output.txt
mpirun -n 8 hw4 5000 5000 8 outputs/output.txt
mpirun -n 8 hw4 5000 5000 8 outputs/output.txt

mpirun -n 10 hw4 5000 5000 10 outputs/output.txt
mpirun -n 10 hw4 5000 5000 10 outputs/output.txt
mpirun -n 10 hw4 5000 5000 10 outputs/output.txt

mpirun -n 16 hw4 5000 5000 16 outputs/output.txt
mpirun -n 16 hw4 5000 5000 16 outputs/output.txt
mpirun -n 16 hw4 5000 5000 16 outputs/output.txt

mpirun -n 20 hw4 5000 5000 20 outputs/output.txt
mpirun -n 20 hw4 5000 5000 20 outputs/output.txt
mpirun -n 20 hw4 5000 5000 20 outputs/output.txt