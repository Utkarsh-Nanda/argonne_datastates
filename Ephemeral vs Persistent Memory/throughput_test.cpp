
// Utkarsh Nanda
// Argonne National Laboratory

#include <omp.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include "vordered_kv.hpp"  

// Function to generate a random string of given length
std::string generate_random_string(int length) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return result;
}

// Function to measure write throughput
template<typename KVStore>
double measure_write_throughput(KVStore& store, bool same_key, int num_operations, int num_threads) {
    std::vector<std::string> keys(num_operations);
    std::vector<std::string> values(num_operations);

    // Generate keys and values
    std::string fixed_key = "fixed_key";
    for (int i = 0; i < num_operations; ++i) {
        keys[i] = same_key ? fixed_key : "key_" + std::to_string(i);
        values[i] = generate_random_string(100);  // 100-character random string
    }

    auto start = std::chrono::high_resolution_clock::now();

    #pragma omp parallel for num_threads(num_threads)
    for (int i = 0; i < num_operations; ++i) {
        store.insert(keys[i], values[i]);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    double throughput = num_operations / elapsed.count();
    return throughput;
}

int main() {
    const int num_operations = 100;
    const int num_threads = omp_get_max_threads();

    // Test with persistent memory store
    vordered_kv_t<std::string, std::string, pmem_history_t<std::string, std::string>> persistent_store("/dev/shm/dstates-db.dat");
    
    // Test with ephemeral memory store
    vordered_kv_t<std::string, std::string, emem_history_t<std::string, std::string>> ephemeral_store("/dev/shm/dstates-db2.dat");

    std::cout << "Testing with " << num_threads << " threads\n";

    // Measure throughput for persistent store
    double persistent_same_key = measure_write_throughput(persistent_store, true, num_operations, num_threads);
    double persistent_diff_keys = measure_write_throughput(persistent_store, false, num_operations, num_threads);

    // Measure throughput for ephemeral store
    double ephemeral_same_key = measure_write_throughput(ephemeral_store, true, num_operations, num_threads);
    double ephemeral_diff_keys = measure_write_throughput(ephemeral_store, false, num_operations, num_threads);

    std::cout << "Persistent store throughput (ops/sec):\n";
    std::cout << "  Same key: " << persistent_same_key << "\n";
    std::cout << "  Different keys: " << persistent_diff_keys << "\n";

    std::cout << "Ephemeral store throughput (ops/sec):\n";
    std::cout << "  Same key: " << ephemeral_same_key << "\n";
    std::cout << "  Different keys: " << ephemeral_diff_keys << "\n";

    return 0;
}
