# CShort

A simple C++ Hello World project built with LLVM.

## Building

To build the project, you need to have LLVM's clang++ compiler installed.

```bash
make
```

This will compile `hello.cpp` and create the `hello` executable.

## Running

After building, run the program:

```bash
./hello
```

Expected output:
```
Hello, World!
```

## Cleaning

To remove the compiled executable:

```bash
make clean
```

## Requirements

- LLVM/clang++ compiler
- Make build system