#include "../svm_commands.h"

#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

bool init_command() {
  if (mkdir(".svm", 0700) != 0) {
  }

  if (mkdir(".svm/objects", 0700) != 0) {
  }

  if (mkdir(".svm/dists", 0700) != 0) {
  }

  FILE *prep = fopen(".svm/prep", "wb");
  if (!prep)
    return false;

  fclose(prep);

  FILE *master_fd = fopen(".svm/dists/master", "w");
  if (!master_fd)
    return false;

  fclose(master_fd);

  return dist_create("master", ".svm/dists/master");
}
