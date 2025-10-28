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

static int get_depth(const char *path) {
  if (path == NULL || *path == '\0') {
    return 0;
  }

  int count = 0;
  const char *p = path;

  if (strncmp(p, "./", 2) == 0) {
    p += 2;
  }

  if (*p == '\0') {
    return 0;
  }

  const char *end = p + strlen(p);

  if (end > p && *(end - 1) == '/') {
    end--;
  }

  if (p == end && *p == '.') {
    return 0;
  }

  if (p < end) {
    count = 0;
    while (p < end) {
      if (*p == '/') {
        count++;
      }
      p++;
    }
  }

  return count;
}

char *add_command() {
  // TODO: La funzione deve leggere index con tutti i file e creare il tree corrispondente

  return "palle";
}
