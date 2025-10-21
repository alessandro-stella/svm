#ifndef SVM_COMMANDS_H
#define SVM_COMMANDS_H

#include <stdbool.h>
#include <stdlib.h>

char *add_command(const char *c);
int clear_command(const char *path);
bool init_command();
char *unpack_command(const char *hex_hash, size_t *out_original_len);

#endif
