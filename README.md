Benchmarking Datastates and RocksDB - It uses YCSB-cpp to benchmark Datastates and RocksDB.

Ephemeral vs Persistent Memory - It has a custom script to benchmark Datastates under two different memory allocation.

Datastates vs RocksDB Exact implementation  - It builds upon the work done in "Ephemeral vs Persistent Memory" to include RocksDB custom benchmarking script as well as updated custom script for Datastates for randomised keys and updates. The implementation includes the same functions being called by YCSB to generate the key, the value, serialization and deserialization.

Benchmarking With YCSB - It has the throughput files on Datastates Ephemeral, Datastates Persistent and RocksDB for read, update and insert operations as a baselin. Tested on the local machine.
