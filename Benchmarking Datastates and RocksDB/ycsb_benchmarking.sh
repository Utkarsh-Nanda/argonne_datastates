
#!/bin/bash

export LD_LIBRARY_PATH=/home/utkarshnanda/deploy/lib64:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/utkarshnanda/ALCFBeginnersGuide/polaris/argonne/rocksdb:$LD_LIBRARY_PATH


# Run benchmarks with varying thread counts
# Get max and steps values from command line arguments

max=$1
steps=$2

# Run benchmarks with varying thread counts
for ((i=$steps; i<=$max; i+=$steps))
do
	echo "Running benchmark with ${i} threads..."
	./ycsb -load -run -db datastates -P workloads/workloada -p "threadcount=${i}" -P dstates/datastates.properties -s >> benchmark.txt

done

echo "Benchmarking complete. Results saved to benchmark.txt."

