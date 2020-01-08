# AccSynt

![](https://github.com/baltoli/accsynt/workflows/Unit%20Tests/badge.svg?branch=master)

AccSynt is a library for automatically learning the behaviour of an accelerator
by querying an interface for input / output examples. It is an experimental
research project in a state of continuous development.

## Building

Build prerequisites:
* CMake 3.2 or newer
* Conan
* A C++ compiler with C++17 support, such as `g++` 7 or newer.

The build instructions in this section assume an environment variable
`$BUILD_ROOT` that points to where you want to build LLVM and AccSynt.

### LLVM 9

```
cd $BUILD_ROOT
git clone git@github.com:llvm/llvm-project
cd llvm-project
git checkout llvmorg-9.0.1
mkdir build
cd build
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=$YOUR_CC \
  -DCMAKE_CXX_COMPILER=$YOUR_CXX \
  -DCMAKE_INSTALL_PREFIX=$BUILD_ROOT/llvm-install \
  -DLLVM_ENABLE_RTTI=On \
  -DLLVM_TARGETS_TO_BUILD=X86 \
  -DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi" \
  ../llvm
make -j`nproc`
make install
```

Note that it's important to build LLVM and AccSynt with the same host compiler
or you will run into library incompatibility issues. The host compiler also
needs to support C++17 for AccSynt.

### AccSynt

```
cd $BUILD_ROOT
git clone https://github.com/Baltoli/accsynt.git
cd accsynt
mkdir build
cd build
cmake \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_COMPILER=$YOUR_CC \
      -DCMAKE_CXX_COMPILER=$YOUR_CXX \
      -DLLVM_DIR=$BUILD_ROOT/llvm-install/lib/cmake/llvm \
      ../src
make -j`nproc`
make install
```

The following conan remotes might be needed in order to build the project:
```
https://api.bintray.com/conan/baltoli/value_ptr
https://api.bintray.com/conan/bincrafters/public-conan
```
