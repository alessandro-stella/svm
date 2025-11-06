#ifndef RESTORE_H
#define RESTORE_H

char *get_tree(char *hash);
bool restore_file_from_blob(char *hash, char *path);

#endif
