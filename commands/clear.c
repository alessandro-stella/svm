#include "../svm_commands.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

int clear_command(const char *path) {
  DIR *d = opendir(path);
  if (!d) {
    perror("opendir");
    return -1;
  }

  struct dirent *entry;
  int ret = 0;

  while ((entry = readdir(d)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    size_t path_len = strlen(path) + strlen(entry->d_name) + 2;
    char *fullpath = malloc(path_len);
    if (!fullpath) {
      perror("malloc");
      ret = -1;
      break;
    }
    snprintf(fullpath, path_len, "%s/%s", path, entry->d_name);

    struct stat st;
    if (stat(fullpath, &st) != 0) {
      perror("stat");
      free(fullpath);
      ret = -1;
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      if (clear_command(fullpath) != 0) {
        ret = -1;
      }
    } else {
      if (unlink(fullpath) != 0) {
        perror("unlink");
        ret = -1;
      }
    }

    free(fullpath);
  }

  closedir(d);

  if (rmdir(path) != 0) {
    perror("rmdir");
    ret = -1;
  }

  return ret;
}
