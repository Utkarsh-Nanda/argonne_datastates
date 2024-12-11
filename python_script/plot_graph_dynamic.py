import matplotlib.pyplot as plt
import numpy as np
import re

def parse_file(filename):
    data = {}
    current_thread = None
    with open(filename, 'r') as f:
        for i, line in enumerate(f):
            try:
                if "Running benchmark with" in line:
                    current_thread = int(re.search(r'(\d+) threads', line).group(1))
                    data[current_thread] = {'insert': {}, 'update': {}, 'read': {}}

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
                        runtime_str = match.group(1)
                        converted_value = runtime_str.replace('e+', 'e-')
                        runtime_init = float(converted_value)
                        runtime_ops = float(match.group(2))
                        runtime_cleanup = float(match.group(3))
                        runtime_total = float(match.group(4))
                        data[current_thread][operation]['init'] = runtime_init
                        data[current_thread][operation]['ops'] = runtime_ops
                        data[current_thread][operation]['cleanup'] = runtime_cleanup
                        data[current_thread][operation]['total'] = runtime_total
            except Exception as e:
                print("Error in line : ", i)
    return data

def create_stacked_bar_chart(rocksdb_data, ephemeral_static_data, ephemeral_dynamic_data):
    # def create_stacked_bar_chart(rocksdb_data, persistent_data, ephemeral_data):
    threads = sorted(set(rocksdb_data) | set(ephemeral_static_data.keys()) | set(ephemeral_dynamic_data.keys()))
    operations = ['insert', 'update', 'read']
    
    x = np.arange(len(threads))
    width = 0.25
    
    fig, axs = plt.subplots(3, 1, figsize=(15, 25))
    
    for i, operation in enumerate(operations):
        rocksdb_init = [rocksdb_data[t][operation].get('init', 0) for t in threads]
        rocksdb_ops = [rocksdb_data[t][operation].get('ops', 0) for t in threads]
        rocksdb_cleanup = [rocksdb_data[t][operation].get('cleanup', 0) for t in threads]
        rocksdb_total = [rocksdb_data[t][operation].get('total', 0) for t in threads]
        
        ephemeral_init = [ephemeral_static_data[t][operation].get('init', 0) for t in threads]
        ephemeral_ops = [ephemeral_static_data[t][operation].get('ops', 0) for t in threads]
        ephemeral_cleanup = [ephemeral_static_data[t][operation].get('cleanup', 0) for t in threads]
        ephemeral_total = [ephemeral_static_data[t][operation].get('total', 0) for t in threads]
        
        dynamic_init = [ephemeral_dynamic_data[t][operation].get('init', 0) for t in threads]
        dynamic_ops = [ephemeral_dynamic_data[t][operation].get('ops', 0) for t in threads]
        dynamic_cleanup = [ephemeral_dynamic_data[t][operation].get('cleanup', 0) for t in threads]
        dynamic_total = [ephemeral_dynamic_data[t][operation].get('total', 0) for t in threads]
        
        
        
        axs[i].bar(x - width, rocksdb_ops, width, label='RocksDB Ops', color='blue')
        axs[i].bar(x - width, rocksdb_cleanup, width, bottom=rocksdb_ops, label='RocksDB Cleanup', color='lightblue')
        axs[i].bar(x - width, rocksdb_init, width, bottom=[sum(x) for x in zip(rocksdb_ops, rocksdb_cleanup)], label='RocksDB Init', color='darkblue')
        
        axs[i].bar(x, ephemeral_ops, width, label='Ephemeral_static Ops', color='red')
        axs[i].bar(x, ephemeral_cleanup, width, bottom=ephemeral_ops, label='Ephemeral static Cleanup', color='lightcoral')
        axs[i].bar(x, ephemeral_init, width, bottom=[sum(x) for x in zip(ephemeral_ops, ephemeral_cleanup)], label='Ephemeral static Init', color='darkred')
        
        axs[i].bar(x + width, dynamic_ops, width, label='Ephemeral_dynamic Ops', color='green')
        axs[i].bar(x + width, dynamic_cleanup, width, bottom=dynamic_ops, label='Ephemeral_dynamic Cleanup', color='lightgreen')
        axs[i].bar(x + width, dynamic_init, width, bottom=[sum(x) for x in zip(dynamic_ops, dynamic_cleanup)], label='Ephemeral_dynamic Init', color='darkgreen')
        
        
        
        for j, thread in enumerate(threads):
            axs[i].text(x[j] - width, rocksdb_total[j], f'{rocksdb_total[j]:.2f}', ha='center', va='bottom')
            axs[i].text(x[j], ephemeral_total[j], f'{ephemeral_total[j]:.2f}', ha='center', va='bottom')
            axs[i].text(x[j] + width, dynamic_total[j], f'{dynamic_total[j]:.2f}', ha='center', va='bottom')
            
        
        axs[i].set_ylabel('Time (seconds)')
        axs[i].set_title(f'{operation.capitalize()} Operation - 8,00,000 ops - Dist - Uniform')
        axs[i].set_xticks(x)
        axs[i].set_xticklabels(threads)
        axs[i].legend()
        # Set y-axis limit to 2e+6
        # axs[i].set_ylim(0, 2e+6)
    
    axs[2].set_xlabel('Number of Threads')
    plt.tight_layout()
    plt.savefig('../plots/benchmark_results_static_dynamic-tail_rocksdb_IUR.png')
    plt.title("RocksDB vs Ephemeral vs Persistent (MultiThreaded - (insert, update, read))")
    plt.show()

# Parse the files
rocksdb_data = parse_file('../throughput_files/benchmark_rocksdb.txt')
# persistent_data = parse_file('../throughput_files/benchmark_ephemeral_dynamic_IRU.txt')
ephemeral_static_data = parse_file('../throughput_files/benchmark_ephemeral_static_insert_read.txt')
ephemeral_dynamic_data = parse_file('../throughput_files/benchmark_ephemeral_dynamic_tail_IRU.txt')
print("RocksDB : ")
print(rocksdb_data)
print("Ephemeral static : ")
print(ephemeral_static_data)
print("Ephemeral Dynamic : ")
print(ephemeral_dynamic_data)

# Create the stacked bar chart
create_stacked_bar_chart(rocksdb_data, ephemeral_static_data, ephemeral_dynamic_data)
# create_stacked_bar_chart(rocksdb_data, persistent_data, ephemeral_data)
