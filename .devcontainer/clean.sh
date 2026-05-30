#!/bin/bash
# This project uses '/tmp/build' for build artifacts by default, which is a temporary directory that is cleared when the container is removed.
echo "-------------------------------------------------------"
BUILD_DIR="/tmp/build"
mkdir -p "$BUILD_DIR"
echo "Cleaning build directory: $BUILD_DIR"
# Clean the build directory
rm -rf "$BUILD_DIR"/*
ls "$BUILD_DIR"

echo "-------------------------------------------------------"