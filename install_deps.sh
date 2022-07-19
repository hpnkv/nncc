set -e

#conan download folly/2022.01.31.00@ -r conancenter -re
#sed -i'' -e 's/if self.settings.os == "Macos" and self.settings.arch != "x86_64":/if False:/g' $HOME/.conan/data/folly/2022.01.31.00/_/_/export/conanfile.py
#
#mkdir external

cd external
#git clone https://github.com/bkaradzic/bgfx.cmake.git
#cd bgfx.cmake
#git submodule init
#git submodule update

cd ../external
git clone https://github.com/CedricGuillemet/ImGuizmo.git ImGuizmo/ImGuizmo
git clone https://github.com/juliettef/IconFontCppHeaders.git IconFontCppHeaders/IconFontCppHeaders
git clone https://github.com/nothings/stb.git stb/stb

git clone https://github.com/cpp-redis/cpp_redis.git
cd cpp_redis
git submodule init
git submodule update