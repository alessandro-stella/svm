#ifndef SVM_COMMANDS_H
#define SVM_COMMANDS_H

#include <stdbool.h>
#include <stdlib.h>

char *add_command(char *msg);

int clear_command(const char *path);
int remove_all_recursive(const char *path);

bool dist_show();
bool dist_create(const char *dist_name, const char *path);

bool init_command();

bool prep_command(char *path);

bool switch_command(const char *dist_name, const char *path);

char *unpack_command(const char *hex_hash, size_t *out_original_len);

#endif
