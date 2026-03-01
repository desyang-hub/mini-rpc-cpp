# mkdir build & cd build & cmake .. & make -j8 # Release
# 由于我本地使用的conda安装protobuf，所以需要启动环境
source ~/miniconda3/etc/profile.d/conda.sh && conda activate tf114 && mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-g -O0' && make -j8