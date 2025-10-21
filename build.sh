#!/bin/bash

# Output executable name
OUTPUT="svm"

# All C source files in the current directory
SOURCES=$(ls *.c)  # svm.c, hashing.c, blob_handler.c

# All C source files inside commands/
COMMANDS_SOURCES=$(ls commands/*.c)

# All C source files inside utils/
UTILS_SOURCES=$(ls utils/*.c)


# Compilation flags
CFLAGS="-Wall -O2"

# Libraries to link
LIBS="-lz -lcrypto"

# Compile all together
gcc $CFLAGS $SOURCES $COMMANDS_SOURCES $UTILS_SOURCES -o $OUTPUT $LIBS

# Check compilation success
if [ $? -eq 0 ]; then
    echo "Compilation successful: ./$OUTPUT"
else
    echo "Compilation failed."
fi
