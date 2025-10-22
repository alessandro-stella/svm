#include "../svm_commands.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool dist_show() {
  FILE *f = fopen(".svm/current_dist", "rb");
  if (!f) {
    printf("Error while retreiving current dist");
    return -1;
  }

  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (file_size <= 0) {
    printf("Error while retreiving current dist (empty file)");
    fclose(f);
    return -1;
  }

  char *current_dist = (char *)malloc(file_size + 1);
  if (!current_dist) {
    perror("malloc");
    fclose(f);
    return -1;
  }

  size_t read_bytes = fread(current_dist, 1, file_size, f);
  fclose(f);

  if (read_bytes != (size_t)file_size) {
    fprintf(stderr, "Error while retreiving current dist\n");
    free(current_dist);
    return -1;
  }

  current_dist[file_size] = '\0';

  const char *dir_path = ".svm/dists/";

  DIR *d;
  struct dirent *dir;

  d = opendir(dir_path);

  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (dir->d_name[0] == '.') {
        continue;
      }

      if (strcmp(current_dist, dir->d_name) == 0)
        printf("(*) ");

      printf("%s\n", dir->d_name);
    }

    closedir(d);
  }

  free(current_dist);
  return true;
}
