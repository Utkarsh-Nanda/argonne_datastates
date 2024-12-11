#!/bin/bash

# Create the directory if it doesn't exist
mkdir -p ../throughput_files

# Output file
OUTPUT_FILE="../throughput_files/custom_throughput_ephemeral_dynamic_IUR.txt"

# Clear the output file if it exists
> "$OUTPUT_FILE"

# Array of thread counts
thread_counts=(1 2 4 8 16)

# Current timestamp for the log
echo "=== Benchmark Run $(date) ===" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Iterate over thread counts
for t in "${thread_counts[@]}"
do
    echo "Running benchmark with $t threads..."
    echo "=== Running with $t threads ===" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
    
    # Run the benchmark and append output to file
    	    rm -r /dev/shm/dbstates.dat
	    ../custom_benchmark_rocksdb $t 3 800000 2>&1 | tee -a >> "$OUTPUT_FILE"
    
    echo "" >> "$OUTPUT_FILE"
    echo "----------------------------------------" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
done

echo "Benchmark complete. Results saved to $OUTPUT_FILE"
