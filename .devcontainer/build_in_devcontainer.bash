#!/bin/bash
# -----------------------------------------------------------------------------
# [BUILD GUIDE FOR DEV CONTAINERS]
#
# This project uses '/tmp/build' for build artifacts to:
# 1. Protect the host file system from pollution (build files stay in the container).
# 2. Minimize Windows memory/disk overhead when using remote servers.
# 3. Ensure a clean environment on every rebuild.
# -----------------------------------------------------------------------------

set -e

# Define the build directory (mirrored with devcontainer.json settings)
BUILD_DIR="/tmp/build"

echo "-------------------------------------------------------"
echo "Project: CShort"
echo "Build Directory: $BUILD_DIR"
echo "-------------------------------------------------------"

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed in this container."
    exit 1
fi

# Configure the project
echo "[1/2] Configuring CMake..."
cmake -S . -B "$BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build the project
echo "[2/2] Building project..."
cmake --build "$BUILD_DIR"

echo "-------------------------------------------------------"
echo "SUCCESS! Build artifacts are located in $BUILD_DIR"
echo "Note: Files in $BUILD_DIR will be deleted when the container is removed."
echo "-------------------------------------------------------"