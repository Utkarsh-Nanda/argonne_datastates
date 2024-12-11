#!/bin/bash


# Run benchmarks with varying thread counts
# Get max and steps values from command line arguments
start_time=$(date +%s.%N)
max=37
steps=4
thread_counts=(1 2 4 8 12 16)
# Run benchmarks with varying thread counts
memory_type="ephemeral_dynamic_tail2_IRU"
for i in "${thread_counts[@]}"
do
	echo "Running benchmark with ${i} threads."
    echo "Running benchmark with ${i} threads." >> ../throughput_files/benchmark_${memory_type}.txt
	echo "Running insert Ops." >> ../throughput_files/benchmark_${memory_type}.txt
    echo "----------------------------------" >> ../throughput_files/benchmark_${memory_type}.txt
    ../ycsb -run -db datastates -P ../workloads/workload_insert -p "threadcount=${i}" -P ../dstates/datastates.properties -s >> ../throughput_files/benchmark_${memory_type}.txt
    echo "Running update Ops." >> ../throughput_files/benchmark_${memory_type}.txt
    echo "----------------------------------" >> ../throughput_files/benchmark_${memory_type}.txt
    ../ycsb -load -run -db datastates -P ../workloads/workload_update -p "threadcount=${i}" -P ../dstates/datastates.properties -s >> ../throughput_files/benchmark_${memory_type}.txt
    #echo "Running read Ops." >> ../throughput_files/benchmark_${memory_type}.txt
    #echo "----------------------------------" >> ../throughput_files/benchmark_${memory_type}.txt
    #../ycsb -load -run -db datastates -P ../workloads/workload_read -p "threadcount=${i}" -P ../dstates/datastates.properties -s >> ../throughput_files/benchmark_${memory_type}.txt
    
	echo "------------------------------------------------------------------------------------------------------------------------------------------" >> ../throughput_files/benchmark_${memory_type}.txt

done

echo "Benchmarking complete. Results saved to throughput_files/benchmark_${memory_type}.txt."

end_time=$(date +%s.%N)

# Calculate the duration
duration=$(echo "$end_time - $start_time" | bc)

echo "Execution time: $duration seconds"
