#!/bin/bash

# Output executable name
OUTPUT="svm"

# All C source files in the current directory
SOURCES=$(ls *.c)

# Compilation flags
CFLAGS="-Wall -O2"  # warnings and optimizations

# Libraries to link
LIBS="-lz -lcrypto"

# Compile
gcc $CFLAGS $SOURCES -o $OUTPUT $LIBS

# Check compilation success
if [ $? -eq 0 ]; then
    echo "Compilation successful: ./$OUTPUT"
else
    echo "Compilation failed."
fi
