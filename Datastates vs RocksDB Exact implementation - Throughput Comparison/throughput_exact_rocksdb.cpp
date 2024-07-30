#include <string>
#include <chrono>
#include <vector>
#include <random>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <future>
#include <atomic>
#include <mutex>
#include "rocksdb_wrapper.hpp"
#include "omp.h"
// Constants
const uint64_t kFNVOffsetBasis64 = 0xCBF29CE484222325ull;
const uint64_t kFNVPrime64 = 1099511628211ull;

// Atomic counter for key generation
class AtomicCounter {
public:
    AtomicCounter(uint64_t initial = 0) : value_(initial) {}
    uint64_t Next() { return value_.fetch_add(1, std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> value_;
};

// Random byte generator
class RandomByteGenerator {
public:
    RandomByteGenerator() : off_(6) {}
    char Next() {
        if (off_ == 6) {
            int bytes = ThreadLocalRandomInt();
            buf_[0] = static_cast<char>((bytes & 31) + ' ');
            buf_[1] = static_cast<char>(((bytes >> 5) & 63) + ' ');
            buf_[2] = static_cast<char>(((bytes >> 10) & 95) + ' ');
            buf_[3] = static_cast<char>(((bytes >> 15) & 31) + ' ');
            buf_[4] = static_cast<char>(((bytes >> 20) & 63) + ' ');
            buf_[5] = static_cast<char>(((bytes >> 25) & 95) + ' ');
            off_ = 0;
        }
        return buf_[off_++];
    }
private:
    char buf_[6];
    int off_;
    static int ThreadLocalRandomInt() {
        static thread_local std::random_device rd;
        static thread_local std::minstd_rand rn(rd());
        return rn();
    }
};

// Hash function
uint64_t FNVHash64(uint64_t val) {
    uint64_t hash = kFNVOffsetBasis64;
    for (int i = 0; i < 8; i++) {
        uint64_t octet = val & 0x00ff;
        val = val >> 8;
        hash = hash ^ octet;
        hash = hash * kFNVPrime64;
    }
    return hash;
}

// Key generation
std::string BuildKeyName(uint64_t key_num, bool ordered_inserts, int zero_padding) {
    if (!ordered_inserts) {
        key_num = FNVHash64(key_num);
    }
    std::string prekey = "user";
    std::string value = std::to_string(key_num);
    int fill = std::max(0, zero_padding - static_cast<int>(value.size()));
    return prekey.append(fill, '0').append(value);
}

// Value generation
struct Field {
    std::string name;
    std::string value;
};

std::vector<Field> BuildValues(int field_count, const std::string& field_prefix, int field_length) {
    std::vector<Field> values;
    RandomByteGenerator byte_generator;
    for (int i = 0; i < field_count; ++i) {
        Field field;
        field.name = field_prefix + std::to_string(i);
        field.value.reserve(field_length);
        std::generate_n(std::back_inserter(field.value), field_length, [&]() { return byte_generator.Next(); });
        values.push_back(field);
    }
    return values;
}

// Serialization
std::string SerializeRow(const std::vector<Field>& values) {
    std::string data;
    for (const Field& field : values) {
        uint32_t len = field.name.size();
        data.append(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
        data.append(field.name);
        len = field.value.size();
        data.append(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
        data.append(field.value);
    }
    return data;
}

// Deserialization
std::vector<Field> DeserializeRow(const std::string& data) {
    std::vector<Field> values;
    const char* p = data.data();
    const char* end = p + data.size();
    while (p < end) {
        Field field;
        uint32_t len = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        field.name = std::string(p, len);
        p += len;
        len = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        field.value = std::string(p, len);
        p += len;
        values.push_back(field);
    }
    return values;
}

// Insert operation
template<typename KVStore>
bool InsertOperation(KVStore& store, const std::string& key, const std::vector<Field>& values) {
    std::string serialized_data = SerializeRow(values);
    try
    {
    store.insert(key, serialized_data);
    }catch (const std::exception& e) {
        std::cerr << "Error in inserting key-value pair. " << e.what() << std::endl;
        return 1;
    }
}

// Update operation
template<typename KVStore>
bool UpdateOperation(KVStore& store, const std::string& key, const std::vector<Field>& new_values) {
    std::string existing_data = store.find(std::numeric_limits<int>::max(), key);
   // if (existing_data == store.low_marker) {
     //   return false;  // Key doesn't exist
   // }
    std::vector<Field> existing_values = DeserializeRow(existing_data);
    for (const auto& new_field : new_values) {
       auto it = std::find_if(existing_values.begin(), existing_values.end(), [&](const Field& field) -> bool {
                                                             return field.name == new_field.name;  
							     });
	    //auto it = std::find_if(existing_values.begin(), existing_values.end(), [&](const Field& f) { return f.name == new_field.name; });
        if (it != existing_values.end()) {
            it->value = new_field.value;
        }
    }
    std::string updated_data = SerializeRow(existing_values);
    
    try {
    store.insert(key, updated_data);
    }catch(std::exception& e) {
     std::cerr << "Error in inserting key-value pair. " << e.what() << std::endl;
        return 1;
    }
}

// Thread-safe statistics
struct Statistics {
    std::atomic<int> insert_count{0};
    std::atomic<int> update_count{0};
    std::atomic<int> total_ops{0};
    std::atomic<int> unique_keys{0};
    std::atomic<int> min_ops_per_key{std::numeric_limits<int>::max()};
    std::atomic<int> max_ops_per_key{0};
    std::mutex map_mutex;
    std::unordered_map<std::string, int> operations_per_key;

    void update(const std::string& key, bool is_insert) {
        if (is_insert) {
            insert_count++;
        } else {
            update_count++;
        }

        int ops;
        {
            std::lock_guard<std::mutex> lock(map_mutex);
            ops = ++operations_per_key[key];
            if (ops == 1) {
                unique_keys++;
            }
        }

        int current_min = min_ops_per_key.load(std::memory_order_relaxed);
        while (ops < current_min && !min_ops_per_key.compare_exchange_weak(current_min, ops, std::memory_order_relaxed));

        int current_max = max_ops_per_key.load(std::memory_order_relaxed);
        while (ops > current_max && !max_ops_per_key.compare_exchange_weak(current_max, ops, std::memory_order_relaxed));

        total_ops++;
    }

    void print() {
        std::cout << "Total inserts: " << insert_count << std::endl;
        std::cout << "Total updates: " << update_count << std::endl;
        std::cout << "Total ops: " << total_ops << std::endl;
        std::cout << "Unique keys: " << unique_keys << std::endl;
        std::cout << "Min operations per key: " << min_ops_per_key << std::endl;
        std::cout << "Max operations per key: " << max_ops_per_key << std::endl;
        std::cout << "Size of operations_per_key: " << operations_per_key.size() << std::endl;
    }
};

// Function to perform operations in a single thread
template<typename KVStore>
int perform_operations(KVStore& store, bool same_key, int start, int end, const std::vector<std::string>& existing_keys, 
                       AtomicCounter& counter, Statistics& stats, int field_count, const std::string& field_prefix, 
                       int field_length, bool ordered_inserts, int zero_padding) {
    int ops_count = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, existing_keys.size() - 1);

    for (int i = start; i < end; ++i) {
        try {
            std::string key;
            bool is_insert;
            if (same_key && !existing_keys.empty()) {
                key = existing_keys[dis(gen)];
                is_insert = false;
            } else {
                key = BuildKeyName(counter.Next(), ordered_inserts, zero_padding);
                is_insert = true;
            }

            std::vector<Field> values = BuildValues(field_count, field_prefix, field_length);

            bool success;
            if (is_insert) {
                success = InsertOperation(store, key, values);
            } else {
                success = UpdateOperation(store, key, values);
            }

            if (success) {
                stats.update(key, is_insert);
                ops_count++;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error performing operation: " << e.what() << std::endl;
        }
    }
    return ops_count;
}

// Function to measure throughput
template<typename KVStore>
double measure_throughput(KVStore& store, bool same_key, int num_operations, int num_threads, int load,
                          int field_count, const std::string& field_prefix, int field_length,
                          bool ordered_inserts, int zero_padding) {
    Statistics stats;
    std::vector<std::string> existing_keys;
    AtomicCounter counter(0);

    // Insert initial load
    if (same_key) {
        for (int i = 0; i < load; ++i) {
            std::string key = BuildKeyName(counter.Next(), ordered_inserts, zero_padding);
            std::vector<Field> values = BuildValues(field_count, field_prefix, field_length);
            if (InsertOperation(store, key, values)) {
                existing_keys.push_back(key);
            }
        }
        std::cout << "Initial load of " << existing_keys.size() << " key-value pairs inserted\n";
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::future<int>> futures;
    int ops_per_thread = num_operations / num_threads;
    int remaining_ops = num_operations % num_threads;

    for (int i = 0; i < num_threads; ++i) {
        int start_op = i * ops_per_thread;
        int end_op = start_op + ops_per_thread + (i == num_threads - 1 ? remaining_ops : 0);
        futures.push_back(std::async(std::launch::async, perform_operations<KVStore>, 
                                     std::ref(store), same_key, start_op, end_op, std::ref(existing_keys),
                                     std::ref(counter), std::ref(stats), field_count, field_prefix, 
                                     field_length, ordered_inserts, zero_padding));
    }

    int total_ops = 0;
    for (auto& future : futures) {
        total_ops += future.get();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    double throughput = total_ops / elapsed.count();
    std::cout << "Completed " << total_ops << " operations\n";
    stats.print();
    return throughput;
}

int main() {
    try {
        const int num_operations = 100000;
        const int num_threads = 1;
        const int initial_load = 100000;
        const int field_count = 10;
        const std::string field_prefix = "field";
        const int field_length = 100;
        const bool ordered_inserts = false;
        const int zero_padding = 20;

        //auto persistent_store = std::make_unique<vordered_kv_t<std::string, std::string, pmem_history_t<std::string, std::string>>>("/dev/shm/persistent_db.dat");
        //auto ephemeral_store = std::make_unique<vordered_kv_t<std::string, std::string, emem_history_t<std::string, std::string>>>("/dev/shm/ephemeral_db.dat");
        auto rocksdb_store = std::make_unique<rocksdb_wrapper_t<std::string, std::string>>("/dev/shm/rocksdb_benchmark");
        std::cout << "Testing with " << num_threads << " threads\n";

        // Persistent store benchmarks
//        std::cout << "\nPersistent store benchmarks:\n";
//        double persistent_insert_diff = measure_throughput(*persistent_store, false, num_operations, num_threads, 0,
                                                           //field_count, field_prefix, field_length, ordered_inserts, zero_padding);
//        double persistent_insert_same = measure_throughput(*persistent_store, true, num_operations, num_threads, initial_load,
                                                           //field_count, field_prefix, field_length, ordered_inserts, zero_padding);

        // Ephemeral store benchmarks
        std::cout << "\nRocksDB benchmarks:\n";
        double rocksdb_insert_diff = measure_throughput(*rocksdb_store, false, num_operations, num_threads, 0,
                                                          field_count, field_prefix, field_length, ordered_inserts, zero_padding);
        double rocksdb_insert_same = measure_throughput(*rocksdb_store, true, num_operations, num_threads, initial_load,
                                                          field_count, field_prefix, field_length, ordered_inserts, zero_padding);

        // Print results
        //std::cout << "\nPersistent store throughput (ops/sec):\n";
        //std::cout << "  Insert (different keys): " << persistent_insert_diff << "\n";
        //std::cout << "  Insert/Update (same keys): " << persistent_insert_same << "\n";

        std::cout << "\nRocksDB store throughput (ops/sec):\n";
        std::cout << "  Insert (different keys): " << rocksdb_insert_diff << "\n";
        std::cout << "  Insert/Update (same keys): " << rocksdb_insert_same << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error in main: " << e.what() << std::endl;
        return 1;
    }
}

