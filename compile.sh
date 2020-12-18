./autogen.sh

./configure

make -j$(nproc)  # 多核make编译
