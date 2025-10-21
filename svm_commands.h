#ifndef SVM_COMMANDS_H
#define SVM_COMMANDS_H

#include <stdlib.h>

char *add_command(const char *c);
int clear_command(const char *path);
bool init_command();
unsigned char *unpack_command(const char *hex_hash, size_t *out_original_len);

#endif
