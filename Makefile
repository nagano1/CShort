# Makefile for building Hello World with LLVM
CXX = clang++
CXXFLAGS = -Wall -Wextra -std=c++11
TARGET = hello
SRC = hello.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
