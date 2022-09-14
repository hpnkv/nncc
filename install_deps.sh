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
mkdir pytorch-build && cd pytorch-build
cmake -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE:STRING=Release -DPYTHON_EXECUTABLE:PATH=`which python3` \
      -DBUILD_PYTHON=OFF -DATEN_NO_TEST=ON -DBUILD_CUSTOM_PROTOBUF=OFF -DUSE_SYSTEM_ONNX=OFF -DBUILD_TEST=0 \
      -DONNX_BUILD_TESTS=ON -DUSE_OPENMP=OFF -DONNX_USE_LITE_PROTO=ON -DONNXIFI_DUMMY_BACKEND=ON -G Ninja \
      -DCMAKE_INSTALL_PREFIX:PATH=../pytorch-install ../pytorch
cmake --build . --target install