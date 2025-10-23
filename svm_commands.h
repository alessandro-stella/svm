#ifndef SVM_COMMANDS_H
#define SVM_COMMANDS_H

#include <stdbool.h>
#include <stdlib.h>

char *add_command();

int clear_command(const char *path);

bool dist_show();
bool dist_create(const char *dist_name, const char *path);

bool init_command();

char prepare_all(const char *path);
char prepare_file(const char *path);

bool switch_command(const char *dist_name, const char *path);

char *unpack_command(const char *hex_hash, size_t *out_original_len);

#endif
