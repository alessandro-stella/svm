#ifndef CONSTANTS_H
#define CONSTANTS_H

// svm.c
#define MAX_PATH_LEN (12 + MAX_DIST_NAME_LEN + 1)
#define COMMANDS_NUMBER 9

// svm.c and switch.c
#define MAX_DIST_NAME_LEN 256

// add.c and blob_handler.c
#define BUF_SIZE 16384

// blob_handler.c
#define INITIAL_DECOMPRESSED_SIZE 1024

// hash_table.c
#define INITIAL_SIZE 101
#define MAX_LOAD_FACTOR 0.75

// switch.c
#define SHA256_HEX_LEN 64

#endif
