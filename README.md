# NNCC
This is a project where I study game engines and rendering, and wire up tools for graphical ML experimentation 
with rich feedback. The name `nncc` is a codename and an acronym for Neural Network Control Centre. However, it will
not be limited to this functionality.

![](https://github.com/apnkv/bgfx-engine/blob/main/demo.gif)

## Build from source
The build process is mostly automated thanks to Conan. However, you will need to download and patch some external 
dependencies, which is done by provided `install_deps.sh`. The whole build sequence is:

```shell
chmod +x install_deps.sh
./install_deps.sh

mkdir build && cd build
conan install ..
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DCMAKE_BUILD_TYPE=Release ..  # this second time is required for -isysroot=macosx to become actual full paths

make -j4
```

After you've done these steps, you'll find built examples in `build/bin`. They now rely on `shaders` folder to be available
in the working directory, so be sure to launch the examples from your terminal.