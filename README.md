# CShort

A simple C++ Hello World project built with LLVM

## Building

### With CMake (recommended)

To build the project using CMake, you need CMake 3.10+ and a C++ compiler installed.

```bash
cmake -B build
cmake --build build
```

This will compile `hello.cpp` and create the `hello` executable inside the `build/` directory.

### With Make

To build the project using Make, you need to have LLVM's clang++ compiler installed.

```bash
make
```

This will compile `hello.cpp` and create the `hello` executable.

## Running

After building with CMake, run the program:

```bash
./build/hello
```

After building with Make, run the program:

```bash
./hello
```

Expected output:
```
Hello, World!
```

## Cleaning

### CMake

```bash
rm -rf build/
```

### Make

To remove the compiled executable:

```bash
make clean
```

## Requirements

- C++ compiler (clang++ or g++)
- CMake 3.10+ (for CMake build)
- Make build system (for Make build)
