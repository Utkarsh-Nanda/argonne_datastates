# Provide the code with the output of benchmarking rocksdb and datastates using YCSB in benchmark.txt

import matplotlib.pyplot as plt
import numpy as np

def read_benchmark_data(filename):
    with open(filename, 'r') as file:
        lines = file.readlines()

    threads = []
    load_throughputs_rocksdb = []
    run_throughputs_rocksdb = []
    load_throughputs_datastates = []
    run_throughputs_datastates = []

    thread_count = 1  # Starting thread count
    step = 1  # Step size

    for line in lines:
        if "Load throughput" in line:
            load_tp = float(line.split(': ')[-1].split()[0])
            if thread_count <= 16:  # Assuming the total thread count for each type is 16
                load_throughputs_rocksdb.append(load_tp)
            else:
                load_throughputs_datastates.append(load_tp)
            threads.append(thread_count)
        elif "Run throughput" in line:
            run_tp = float(line.split(': ')[-1].split()[0])
            if thread_count <= 16:
                run_throughputs_rocksdb.append(run_tp)
            else:
                run_throughputs_datastates.append(run_tp)
            thread_count += step

    return threads[:len(threads)//2], load_throughputs_rocksdb, run_throughputs_rocksdb, load_throughputs_datastates, run_throughputs_datastates

def plot_throughput(threads, load_rdb, run_rdb, load_dst, run_dst):
    width = 0.2  # Width of the bars
    fig, ax = plt.subplots(figsize=(14, 7))

    # Creating bar positions
    r1 = np.arange(len(threads))  # Positions for RocksDB bars
    r2 = [x + width for x in r1]  # Shift positions for DataStates bars
    r3 = [x + width for x in r2]  # Further shift
    r4 = [x + width for x in r3]  # Further shift

    # Creating bars
    ax.bar(r1, load_rdb, color='b', width=width, edgecolor='grey', label='Load-RocksDB')
    ax.bar(r2, run_rdb, color='r', width=width, edgecolor='grey', label='Run-RocksDB')
    ax.bar(r3, load_dst, color='g', width=width, edgecolor='grey', label='Load-DataStates')
    ax.bar(r4, run_dst, color='y', width=width, edgecolor='grey', label='Run-DataStates')

    # Adding labels and title
    ax.set_xlabel('Threads', fontweight='bold', fontsize=12)
    ax.set_ylabel('Throughput (ops/sec)', fontweight='bold', fontsize=12)
    ax.set_title('Threads vs Throughput for RocksDB and DataStates')
    ax.set_xticks([r + width + width/2 for r in range(len(threads))])
    ax.set_xticklabels(threads)

    ax.legend()

    plt.savefig('threads_vs_throughput.png')

filename = 'benchmark.txt'
threads, load_rdb, run_rdb, load_dst, run_dst = read_benchmark_data(filename)
plot_throughput(threads, load_rdb, run_rdb, load_dst, run_dst)
