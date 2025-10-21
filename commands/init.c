#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

bool init_command() {
  mkdir(".svm", 0700);
  mkdir(".svm/objects", 0700);
  mkdir(".svm/dists", 0700);

  FILE *current_dist = fopen(".svm/current_dist", "wb");

  size_t content_len = strlen("master");
  if (fwrite("master", 1, content_len, current_dist) != content_len) {
    fclose(current_dist);
    return false;
  }

  if (current_dist == NULL) {
    return false;
  }

  if (fclose(current_dist) != 0) {
    printf("Error during fclose");
    return false;
  }

  return true;
}
