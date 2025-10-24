#include "../svm_commands.h"

#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *join_path(const char *base, const char *subdir) {
  if (!base)
    base = "";
  if (!subdir)
    subdir = "";

  int need_slash = (base[strlen(base) - 1] != '/');

  size_t len = strlen(base) + strlen(subdir) + (need_slash ? 1 : 0) + 1;

  char *path = malloc(len);
  if (!path) {
    perror("malloc");
    return NULL;
  }

  strcpy(path, base);
  if (need_slash)
    strcat(path, "/");
  strcat(path, subdir);

  return path;
}

char *add_command() {
  // TODO: La funzione deve leggere index con tutti i file e creare il tree corrispondente

  return "palle";
}
