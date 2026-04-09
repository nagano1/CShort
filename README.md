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

This project is optimized for **VS Code + Dev Containers** to ensure a clean and high-performance development environment.

### Build Artifacts Strategy
To protect your host machine from file pollution and to minimize memory/disk overhead (especially when using remote SSH), all build artifacts are stored in a **temporary directory** inside the container:

- **Build Directory:** `/tmp/build`
- **Benefit:** Build files (objects, binaries) never touch your local repository or host disk. They are automatically cleaned up when the container is removed.

### Quick Build
Once the Dev Container is started, you can build the project using the following script:
```bash
bash .devcontainer/build_in_devcontainer.bash