import matplotlib.pyplot as plt

def parse_results(filename):
    results = {}
    current_threads = None
    current_store = None
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith("Testing with"):
                current_threads = int(line.split()[2])
                results[current_threads] = {}
            elif "Persistent" in line:
                current_store = "persistent"
            elif "Ephemeral" in line:
                current_store = "ephemeral"
            elif "Same key:" in line:
                value = float(line.split(":")[1].strip())
                results[current_threads][f'{current_store}_same'] = value
            elif "Different keys:" in line:
                value = float(line.split(":")[1].strip())
                results[current_threads][f'{current_store}_diff'] = value
    return results

def create_bar_graph(results):
    threads = sorted(results.keys())
    ephemeral_diff = [results[t]['ephemeral_diff'] for t in threads]
    persistent_diff = [results[t]['persistent_diff'] for t in threads]
    ephemeral_same = [results[t]['ephemeral_same'] for t in threads]
    persistent_same = [results[t]['persistent_same'] for t in threads]

    x = range(len(threads))
    width = 0.2

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.bar([i - 1.5*width for i in x], ephemeral_diff, width, label='Ephemeral (Different Keys)', color='blue')
    ax.bar([i - 0.5*width for i in x], persistent_diff, width, label='Persistent (Different Keys)', color='lightblue')
    ax.bar([i + 0.5*width for i in x], ephemeral_same, width, label='Ephemeral (Same Key)', color='red')
    ax.bar([i + 1.5*width for i in x], persistent_same, width, label='Persistent (Same Key)', color='lightcoral')

    ax.set_ylabel('Throughput (ops/sec)')
    ax.set_xlabel('Number of Threads')
    ax.set_title('Throughput Comparison')
    ax.set_xticks(x)
    ax.set_xticklabels(threads)
    ax.legend()

    plt.tight_layout()
    plt.savefig('throughput_comparison.png')
    plt.close()

results = parse_results('throughput_test_res.txt')
create_bar_graph(results)
