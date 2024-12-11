#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <map>
#include <omp.h>
#include <chrono>
#include "dstates/vordered_kv.hpp"

// Constants
int N = 10;       // Number of key-value pairs
const int M = 10; // Length of random strings
const std::string DB_PATH = "/dev/shm/dbstates.dat";
std::vector<std::pair<std::string, std::string>> key_value;
std::vector<std::pair<int, std::string>> updates;
// Map to store keys and their history timestamps
std::map<std::string, std::vector<int>> key_history_timestamps;
// Utility function to generate random string of length M
std::string generate_random_string(std::mt19937 &rng)
{
    static const char alphanum[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<> dist(0, sizeof(alphanum) - 2);
    std::string str;
    str.reserve(M);
    for (int i = 0; i < M; ++i)
    {
        str += alphanum[dist(rng)];
    }
    return str;
}

int zipfian_next(int n, double alpha = 1.0, unsigned int seed = 42)
{
    static std::mt19937 rng(seed);
    static std::vector<double> cdf;
    static int last_n = -1;
    static double last_alpha = -1.0;

    // Recompute CDF only if n or alpha changed
    if (n != last_n || alpha != last_alpha)
    {
        // Compute normalization constant
        double c = 0;
        for (int i = 1; i <= n; i++)
        {
            // Use (n-i+1) instead of i to prioritize ending elements
            c += 1.0 / std::pow(n - i + 1, alpha);
        }

        // Compute CDF
        cdf.resize(n + 1);
        cdf[0] = 0;
        for (int i = 1; i <= n; i++)
        {
            // Use (n-i+1) to invert the probability distribution
            cdf[i] = cdf[i - 1] + (1.0 / std::pow(n - i + 1, alpha)) / c;
        }

        last_n = n;
        last_alpha = alpha;
    }

    // Generate uniform random number between 0 and 1
    std::uniform_real_distribution<> dist(0.0, 1.0);
    double u = dist(rng);

    // Binary search to find the value
    int rank = std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin();

    // Convert the rank back to the actual index (n - rank) to prioritize ending elements
    return rank;
}

int uniform_next(int n, unsigned int seed = 42)
{
    static std::mt19937 rng(seed);
    std::uniform_int_distribution<> dist(0, n - 1);
    return dist(rng);
}

// Global storage for keys and their history counts
std::map<std::string, int> key_history_counts;
std::shared_mutex history_mutex;

// Function to insert key-value pairs
template <typename Map>
void run_insert(Map &vmap, const int n, const int t)
{
    std::cout << "Starting insertion with " << t << " threads..." << " for " << n << " inserts." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    std::atomic<int> count{0};
#pragma omp parallel num_threads(t)
    {
#pragma omp for
        for (int i = 0; i < n; i++)
        {
            std::string key = key_value[i].first;
            std::string value = key_value[i].second;
            /*
            int version;
            {
                std::unique_lock lock(history_mutex);
                version = (int) vmap.tag();
        //vmap.insert(key, value, version);
            }*/

            /*{
             std::unique_lock lock(history_mutex);
            std::cout << "key : " << key << " value : " << value << "\n";
                //vmap.tag();
            }*/
            // vmap.insert(key, value, version);
            count++;
            int timestamp = vmap.tag();
            vmap.insert(key, value, true);
            /*{
                std::unique_lock<std::mutex> lock(history_mutex);
                key_history_counts[key]++;
            }*/
        }
    }
    std::cout << "Insert completed : " << count << "\n";
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Insertion completed in " << duration.count() << "ms" << std::endl;
}

// Function to update existing key-value pairs
template <typename Map>
void run_update(Map &vmap, const int n, const int t)
{
    std::cout << "Starting updates with " << t << " threads..." << " for : " << n << " updates." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

#pragma omp parallel num_threads(t)
    {
#pragma omp for
        for (int i = 0; i < n; i++)
        {

            std::string key = key_value[updates[i].first].first; // get the key from it's index
            std::string new_value = updates[i].second;           // get the new value from the updates vector
                                                                 /*
                                                                int version;
                                                             {
                                                                     std::unique_lock lock(history_mutex);
                                                                     version = (int) vmap.tag();
                                                                     //vmap.insert(key, new_value, version);
                                                             }
                                                             */
            vmap.insert(key, new_value, true);
            // vmap.insert(key, new_value, version);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Updates completed in " << duration.count() << "ms" << std::endl;
}

// Function to read key-value pairs
template <typename Map>
void run_read(Map &vmap, const int n, const int t, std::vector<std::pair<std::string, int>> &reads)
{
    std::cout << "Starting reads with " << t << " threads..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    // std::atomic<int> successful_reads{0};
    // std::atomic<int> total_versions_read{0};

#pragma omp parallel num_threads(t)
    {
        std::vector<std::pair<int, std::string>> history;

#pragma omp for
        for (size_t i = 0; i < reads.size(); i++)
        {
            const auto &read_pair = reads[i];
            const std::string &key = read_pair.first;
            int version = read_pair.second;
            // int version = 10;
            // Get the value for the specific key and version
            // std::cout << "To search : key : " << key << " version : " << version << "\n";
            auto value = vmap.find(version, key);
            // std::cout << "Value for key " << key << " and version " << version << " is " << value << std::endl;
            // if (value != vordered_kv_t<std::string, std::string>::low_marker) {
            //     successful_reads++;

            // Get history size for statistics
            // history.clear();
            // vmap.get_key_history(key, history);
            // total_versions_read += history.size();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // // Print statistics
    std::cout << "Read benchmark completed in " << duration.count() << "ms" << std::endl;
    // std::cout << "Successful reads: " << successful_reads << "/" << reads.size() << std::endl;
    // if (successful_reads > 0) {
    //     double avg_versions = static_cast<double>(total_versions_read) / successful_reads;
    //     std::cout << "Average versions per key: " << avg_versions << std::endl;
    // }
}

// Function to create reference key-value pairs
void create_reference(const int n)
{
    std::cout << "Creating reference key-value pairs..." << std::endl;
    std::mt19937 rng(42);
    for (int i = 0; i < n; i++)
    {
        std::string key = generate_random_string(rng);
        std::string value = generate_random_string(rng);
        key_value.push_back({key, value});
        key_history_counts[key]++;
        // std::cout << "Key inserted : " << key << "\n";
    }
}

void create_update_indexes(const int n)
{
    std::cout << "Creating update indexes..." << std::endl;
    std::mt19937 rng(42);
    for (int i = 0; i < n; i++)
    {
        int key = uniform_next(n);
        key = key == n ? n - 1 : key;
        // std::cout << "Key index : " << key << "\n";
        std::string value = generate_random_string(rng);
        updates.push_back({key, value});
        key_history_counts[key_value[key].first]++;
        // std::cout << "Key Updated : " << key_value[key].first << " value : " << value << "\n";
    }
}

void create_reads(const int n, std::vector<std::pair<std::string, int>> &reads)
{
    std::cout << "Creating read operations with Zipfian distribution..." << std::endl;
    // Create vector of keys for zipfian selection
    std::vector<std::string> keys;
    keys.reserve(key_history_counts.size());
    for (const auto &pair : key_history_counts)
    {
        keys.push_back(pair.first);
    }

    std::mt19937 rng(42);
    for (int i = 0; i < n; i++)
    {
        // Select key using zipfian distribution, among the keys inserted
        int key_index = uniform_next(keys.size());

        std::string selected_key = keys[key_index];

        // Get history size for the selected key
        int history_size = key_history_counts[selected_key];

        // Select version using zipfian distribution if history exists
        int selected_version_index = 0;
        if (history_size > 0)
        {
            selected_version_index = uniform_next(history_size);
        }
        // get the version from the key_history_timestamps map, based on the index
        int selected_version = key_history_timestamps[selected_key][selected_version_index];
        reads.emplace_back(selected_key, selected_version);
    }
    std::cout << "Read operations created: " << reads.size() << std::endl;
}

template <typename Map>
void populate_key_history_timestamps(Map &vmap)
{
    // Iterate over all keys in vmap
    std::vector<std::string> keys;

    for (auto &key_pair : key_value)
    {
        const std::string &key = key_pair.first;
        std::vector<std::pair<int, std::string>> history;
        vmap.get_key_history(key, history);

        // Extract timestamps from the history
        std::vector<int> timestamps;
        for (const auto &entry : history)
        {
            timestamps.push_back(entry.first);
        }

        // Store the timestamps in the map
        {
            key_history_timestamps[key] = timestamps;
        }
    }
}
int main(int argc, char **argv)
{
    if (argc != 4)
    {
        std::cout << "Usage: " << argv[0] << " <num_threads> <operation_type>" << std::endl;
        std::cout << "operation_type: 1=insert, 2=update, 3=read" << std::endl;
        return 1;
    }

    int num_threads = std::stoi(argv[1]);
    int operation = std::stoi(argv[2]);
    N = std::stoi(argv[3]);
    // Create vordered_kv instance
    vordered_kv_t<std::string, std::string, emem_history_t<std::string, std::string>> vmap(DB_PATH);

    std::vector<std::pair<std::string, int>> reads;
    if (operation == 1)
    {
        create_reference(N);
        run_insert(vmap, N, num_threads);
    }
    else if (operation == 2)
    {
        create_reference(N);
        run_insert(vmap, N, num_threads);
        create_update_indexes(N);
        run_update(vmap, N, num_threads);
    }
    else if (operation == 3)
    {
        create_reference(N);
        run_insert(vmap, N, num_threads);
        create_update_indexes(N);
        run_update(vmap, N, num_threads);
        populate_key_history_timestamps(vmap);
        create_reads(N, reads);
        run_read(vmap, N, num_threads, reads);
    }
    else
    {
        std::cout << "Invalid operation type" << std::endl;
        return 1;
    }

    // Print statistics
    // std::cout << "\nKey History Statistics Actual: " << std::endl;
    // int count = 0;
    // for (const auto& pair : key_history_counts) {
    //         count += pair.second;
    //     std::cout << "Key: " << pair.first << ", History Count: " << pair.second << std::endl;
    // }
    // std::cout << "Count : " << count << "\n";

    // // Print statistics
    // std::cout << "\nKey History Statistics:" << std::endl;

    // int history_total_size = 0;
    // for (const auto& pair : key_history_counts) {
    //     std::vector<std::pair<int, std::string>> history;
    //     vmap.get_key_history(pair.first, history);
    // history_total_size += history.size();
    //     std::cout << "Key: " << pair.first << ", History Size: " << history.size() << std::endl;
    //     for (const auto& h : history) {
    //         std::cout << "  Version: " << h.first << ", Value: " << h.second << std::endl;
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "Total history size combined : " << history_total_size << "\n";

    return 0;
}
