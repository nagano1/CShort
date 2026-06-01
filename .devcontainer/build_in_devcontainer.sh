#!/bin/bash
# This project uses '/tmp/build' for build artifacts by default, which is a temporary directory that is cleared when the container is removed.
echo "-------------------------------------------------------"
BUILD_DIR="/tmp/build/"
echo "Build Directory: $BUILD_DIR"

# Configure the project
echo "[1/2] Configuring CMake..."
cmake -B "$BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# Build the project
echo "[2/2] Building project..."
cmake --build "$BUILD_DIR" -j$(nproc)
echo "SUCCESS! Build artifacts are located in $BUILD_DIR"
echo "-------------------------------------------------------"