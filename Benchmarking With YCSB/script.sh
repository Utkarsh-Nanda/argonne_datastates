#!/bin/bash


./ycsb -load -run -db datastates -P workloads/workload_read -P dstates/datastates.properties -s >> throughput_datastates_persistent.txt

echo "---------------------------------------------------------------------" >> throughput_datastates_persistent.txt

./ycsb -load -run -db datastates -P workloads/workload_insert -P dstates/datastates.properties -s >> throughput_datastates_persistent.txt

echo "---------------------------------------------------------------------" >> throughput_datastates_persistent.txt

./ycsb -load -run -db datastates -P workloads/workload_update -P dstates/datastates.properties -s >> throughput_datastates_persistent.txt



echo "Completed."
