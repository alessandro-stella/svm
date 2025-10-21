#ifndef DIRECTORY_HANDLER_H
#define DIRECTORY_HANDLER_H

#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

char *create_tree(const char *c, size_t *tree_size);
int remove_dir(const char *path);

#endif
