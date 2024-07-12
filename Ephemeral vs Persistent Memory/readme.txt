Keep all the files in the YCSB-cpp directory.

To run the code first setup the environment by running : 

module avail gnu
source ./config.sh

Then setup the python environment by running : 

module use /soft/modulefiles; module load conda ; conda activate base

Then to compile "throughput_benchmark.cpp" : 

bash ./throughput_benchmark.sh
