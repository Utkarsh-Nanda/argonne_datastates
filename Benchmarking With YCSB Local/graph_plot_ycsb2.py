import matplotlib.pyplot as plt
import numpy as np

def parse_file(filename):
    data = {}
    current_threads = None
    with open(filename, 'r') as f:
        ops = ""
        for line in f:
            if line.startswith("Running benchmark with"):
                current_threads = int(line.split()[3])
                data[current_threads] = {'Read': 0, 'Insert': 0, 'Update': 0, 'Read_max': 0, 'Insert_max': 0, 'Update_max': 0}
            elif "READ:" in line:
                ops = "Read"
            elif "INSERT:" in line:
                ops = "Insert"
            elif "UPDATE:" in line:
                ops = "Update"
            elif "Run throughput(ops/sec):" in line:
                value = float(line.split(":")[1].strip())
                data[current_threads][ops] = value
            elif "Run throughput(ops/sec) based on the max time taken by any thread:" in line:
                value = float(line.split(":")[1].strip())
                ops2 = ops + "_max"
                data[current_threads][ops2] = value
    return data

ephemeral_data = parse_file('throughput_datastates_ephemeral_multi2.txt')
persistent_data = parse_file('throughput_datastates_persistent_multi2.txt')
rocksdb_data = parse_file('throughput_rocksdb_multi2.txt')

def create_plot(operation, max_time=False):
    threads = sorted(set(ephemeral_data.keys()) | set(persistent_data.keys()) | set(rocksdb_data.keys()))

    ephemeral_values = [ephemeral_data.get(t, {}).get(operation + ('_max' if max_time else ''), 0) for t in threads]
    persistent_values = [persistent_data.get(t, {}).get(operation + ('_max' if max_time else ''), 0) for t in threads]
    rocksdb_values = [rocksdb_data.get(t, {}).get(operation + ('_max' if max_time else ''), 0) for t in threads]

    x = np.arange(len(threads))
    width = 0.25

    fig, ax = plt.subplots(figsize=(15, 8))
    rects1 = ax.bar(x - width, ephemeral_values, width, label='Ephemeral', color='red')
    rects2 = ax.bar(x, persistent_values, width, label='Persistent', color='green')
    rects3 = ax.bar(x + width, rocksdb_values, width, label='RocksDB', color='blue')

    ax.set_ylabel('Throughput (ops/sec)')
    ax.set_xlabel('Number of Threads')
    ax.set_title(f'{operation} Throughput {"(Only Operations Considered)" if max_time else ""}')
    ax.set_xticks(x)
    ax.set_xticklabels(threads)
    ax.legend()

    # Function to add value labels
    def autolabel(rects):
        for rect in rects:
            height = rect.get_height()
            ax.annotate(f'{height:.0f}',
                        xy=(rect.get_x() + rect.get_width() / 2, height),
                        xytext=(0, 3),  # 3 points vertical offset
                        textcoords="offset points",
                        ha='center', va='bottom', rotation=90)

    autolabel(rects1)
    autolabel(rects2)
    autolabel(rects3)

    plt.tight_layout()
    plt.savefig(f'.//graphs_correct//{operation.lower()}_throughput{"_only_ops" if max_time else ""}.png')
    plt.close()

# Generate plots
for op in ['Read', 'Insert', 'Update']:
    create_plot(op)
    create_plot(op, max_time=True)

print("ephemeral data : \n")
print(ephemeral_data)

print("persistent data : \n")
print(persistent_data)

print("rocksdb data : \n")
print(rocksdb_data)

print("Graphs have been generated and saved.")

