Hailo Playground
===

## Building

### Dependencies
OpenCV

HailoRT

### Binaries
CMake is the metabuild system used.

Debug builds are default
```
git clone https://github.com/iancohee/hailo_playground.git
cd hailo_playground
mkdir build
cd build
cmake ..
make
```

Release builds can be specified
```
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```