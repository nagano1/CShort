# CShort

A simple C++ Hello World project built with CMake

## Building

To build the project, you need CMake 3.10+ and a C++ compiler installed.

```bash
cmake -B build
cmake --build build
```

This will compile `hello.cpp` and create the `hello` executable inside the `build/` directory.

## Running

After building, run the program:

```bash
./build/hello
```

Expected output:
```
Hello, World!
```

## Cleaning

```bash
rm -rf build/
```

## Requirements

- C++ compiler (clang++ or g++)
- CMake 3.10+


## 🛠 Development in Dev Containers

To protect your host machine from file pollution , all build artifacts are stored in /tmp by default inside the container:
```bash
/tmp/build`
```

### Quick Build
Once the Dev Container is started, you can build the project using the following script:
```bash
bash .devcontainer/build_in_devcontainer.bash