set -e

conan download folly/2022.01.31.00@ -r conancenter -re
sed -i'' -e 's/if self.settings.os == "Macos" and self.settings.arch != "x86_64":/if False:/g' \
    "$HOME"/.conan/data/folly/2022.01.31.00/_/_/export/conanfile.py

mkdir external

cd external
git clone https://github.com/bkaradzic/bgfx.cmake.git
cd bgfx.cmake
git submodule init
git submodule update

cd ../
git clone https://github.com/CedricGuillemet/ImGuizmo.git ImGuizmo/ImGuizmo
git clone https://github.com/juliettef/IconFontCppHeaders.git IconFontCppHeaders/IconFontCppHeaders
git clone https://github.com/nothings/stb.git stb/stb

git clone https://github.com/cpp-redis/cpp_redis.git
cd cpp_redis
git submodule init
git submodule update

cd ../
git clone -b master --recurse-submodule https://github.com/pytorch/pytorch.git
cd pytorch
BUILD_SHARED_LIBS=OFF BUILD_BINARY=OFF ATEN_NO_TEST=ON BUILD_TEST=0 USE_SYSTEM_ONNX=OFF BUILD_CUSTOM_PROTOBUF=OFF USE_PROTOBUF_SHARED_LIBS=ON ONNX_BUILD_TESTS=ON USE_OPENMP=OFF python tools/build_libtorch.py
sed -i'' -e 's/add_library(shm SHARED core.cpp)/add_library(shm STATIC core.cpp)/g' \
    "$HOME"/.conan/data/folly/2022.01.31.00/_/_/export/conanfile.py
sed -i'' -e 's/target_link_libraries(shm torch c10)/target_link_libraries(shm torch)/g' \
    torch/lib/libshm/CMakeLists.txt