#!/bin/bash

g++ -std=c++17 -fopenmp throughput_test.cpp -o throughput_test \
    -I/home/utkarshnanda/deploy/include/dstates/ \
    -I/home/utkarshnanda/local/include \
    -I/home/utkarshnanda/deploy/include \
    -L/home/utkarshnanda/deploy/lib64 \
    -L/home/utkarshnanda/local/lib \
    -lpmemobj -lpmem -pthread

# Run the test for different thread counts
for threads in 1 2 4 8 16 32 64 128; do
    export OMP_NUM_THREADS=$threads
    echo "Running with $threads threads" >> throughput_test_res.txt
    ./throughput_test >> throughput_test_res.txt
    echo "" >> throughput_test_res.txt
done
