To run the code first setup the environment by running : 

moudle avail gnu
source ./config.sh

Then setup the python environment by running : 

module use /soft/modulefiles; module load conda ; conda activate base

Then to compile "throughput_benchmark.cpp" : 

bash ./throughput_benchmark.sh