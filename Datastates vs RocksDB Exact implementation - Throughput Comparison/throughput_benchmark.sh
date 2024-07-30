#!/bin/bash/

g++ -std=c++17 -fopenmp throughput_exact_datastates.cpp -o throughput_exact_datastates -fopenmp -I/home/utkarshnanda/ALCFBeginnersGuide/polaris/argonne/rocksdb/include -L/home/utkarshnanda/ALCFBeginnersGuide/polaris/argonne/rocksdb -lrocksdb -lpthread -I/home/utkarshnanda/deploy/include/dstates/ -I/home/utkarshnanda/deploy/include -lpmem -lpmemobj -L/home/utkarshnanda/local/lib -L/home/utkarshnanda/deploy/lib64 -I/home/utkarshnanda/local/include

echo "Compilation Done."

# Run the test for different thread counts
for threads in 1 2 4 8 16 32 64 128; do
    export OMP_NUM_THREADS=$threads
    echo "Running with $threads threads"
    echo "Running with $threads threads" >> throughput_test_res2.txt
    #rm /dev/shm/rocksdb_benchmark -r
    rm /dev/shm/persistent_db.dat
    ./throughput_exact_datastates >> throughput_test_res2.txt
    echo "------------------------------------------------------------------------------------" >> throughput_test_res2.txt
done
