import matplotlib.pyplot as plt
import numpy as np
import re

def parse_file(filename):
    data = {}
    current_thread = None
    with open(filename, 'r') as f:
        for i, line in enumerate(f):
            try:
                throughput = 0.0
                ops_time = 0.0
                if "Running benchmark with" in line:
                    current_thread = int(re.search(r'(\d+) threads', line).group(1))
                    data[current_thread] = {}

                elif "Running insert Ops." in line:
                    operation = 'insert'

                elif "Running update Ops." in line:
                    operation = 'update'

                elif "Running read Ops." in line:
                    operation = 'read'
                elif "Runtime Init :" in line:
                    pattern = r"Runtime Init : (\S+) Runtime Ops : (\S+) Runtime Cleanup : (\S+) Runtime Total : (\S+)"
                    match = re.search(pattern, line)
                    if match:
                        ops_time = float(match.group(2))
                        data[current_thread][operation]= 800000 / ops_time
                    
            except Exception as e:
                print("Error in line : ", i)
    return data

def create_stacked_bar_chart(rocksdb_data, ephemeral_static_data, rocksdb_dynamic_data):
    # def create_stacked_bar_chart(rocksdb_data, persistent_data, ephemeral_data):
    threads = sorted(set(rocksdb_data.keys()) | set(ephemeral_static_data.keys()) | set(ephemeral_static_data.keys()))
    operations = ['insert', 'update', 'read']
    
    x = np.arange(len(threads))
    width = 0.25
    
    fig, axs = plt.subplots(3, 1, figsize=(15, 25))
    
    for i, operation in enumerate(operations):

        # ephemeral_throughput = [ephemeral_data[t][operation] for t in threads]
        ephemeral_static_throughput = [ephemeral_static_data[t].get(f'{operation}', 0) for t in threads]
        
        ephemeral_dynamic_throughput = [ephemeral_dynamic_data[t][operation] for t in threads]

        rocksdb_throughput = [rocksdb_data[t][operation] for t in threads]
        
        # ephemeral_throughput = [ephemeral_data[t][operation] for t in threads]
        
        axs[i].bar(x - width, rocksdb_throughput, width, label='rocksdb static Ops Throughput', color='blue')
        # axs[i].bar(x - width, rocksdb_cleanup, width, bottom=rocksdb_ops, label='RocksDB Cleanup', color='lightblue')
        # axs[i].bar(x - width, rocksdb_init, width, bottom=[sum(x) for x in zip(rocksdb_ops, rocksdb_cleanup)], label='RocksDB Init', color='darkblue')
        
        axs[i].bar(x, ephemeral_static_throughput, width, label='ephemeral static Ops Throughput', color='red')


        axs[i].bar(x + width, ephemeral_dynamic_throughput, width, label='Ephemeral Dynamic Ops Throughput', color='green')
        # axs[i].bar(x, persistent_cleanup, width, bottom=persistent_ops, label='Persistent Cleanup', color='lightgreen')
        # axs[i].bar(x, persistent_init, width, bottom=[sum(x) for x in zip(persistent_ops, persistent_cleanup)], label='Persistent Init', color='darkgreen')
        
        
        
        # axs[i].bar(x + width, ephemeral_throughput, width, label='Ephemeral Ops Throughput', color='red')
        # axs[i].bar(x + width, ephemeral_cleanup, width, bottom=ephemeral_ops, label='Ephemeral Cleanup', color='lightcoral')
        # axs[i].bar(x + width, ephemeral_init, width, bottom=[sum(x) for x in zip(ephemeral_ops, ephemeral_cleanup)], label='Ephemeral Init', color='darkred')
        
        # for j, thread in enumerate(threads):
        #     axs[i].text(x[j] - width, rocksdb_throughput[j], ha='center', va='bottom')
        #     axs[i].text(x[j], persistent_throughput[j], ha='center', va='bottom')
        #     axs[i].text(x[j] + width, ephemeral_throughput[j], ha='center', va='bottom')
        
        axs[i].set_ylabel('Throughput (ops/sec)')
        axs[i].set_title(f'{operation.capitalize()} Operation')
        axs[i].set_xticks(x)
        axs[i].set_xticklabels(threads)
        axs[i].legend()
        # Set y-axis limit to 2e+6
        axs[i].set_ylim(0, 1e+6)
    
    axs[2].set_xlabel('Number of Threads')
    plt.tight_layout()
    plt.savefig('../plots/benchmark_results_throughput_static_dynamic_prev_rocksdb_IUR.png')
    # plt.title("RocksDB vs Ephemeral vs Persistent (MultiThreaded - (insert, update, read))")
    plt.show()

# Parse the files
rocksdb_data = parse_file('../throughput_files/benchmark_rocksdb.txt')
ephemeral_static_data = parse_file('../throughput_files/benchmark_ephemeral_static_insert_read.txt')
ephemeral_dynamic_data = parse_file('../throughput_files/benchmark_ephemeral_dynamic_prev_IRU.txt')
# ephemeral_data = parse_file('benchmark_ephemeral.txt')
# print("RocksDB : ")
# print(rocksdb_data)
# print("Persistent : ")
# print(persistent_data)
print("Rocksdb : ")
print(rocksdb_data)
print("ephemeral static : ")
print(ephemeral_static_data)
print("Ephemeral Dynamic: ")
print(ephemeral_dynamic_data)


# Create the stacked bar chart
create_stacked_bar_chart(rocksdb_data, ephemeral_static_data, ephemeral_dynamic_data)
