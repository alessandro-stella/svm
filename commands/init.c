#include "../svm_commands.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

bool init_command() {
  if (mkdir(".svm", 0700) != 0) {
  }

  if (mkdir(".svm/objects", 0700) != 0) {
  }

  if (mkdir(".svm/dists", 0700) != 0) {
  }

  FILE *current_dist = fopen(".svm/current_dist", "wb");
  if (!current_dist)
    return false;

  if (fwrite("master", 1, strlen("master"), current_dist) != strlen("master")) {
    fclose(current_dist);
    return false;
  }
  fclose(current_dist);

  FILE *prep = fopen(".svm/prep", "wb");
  if (!prep)
    return false;

  fclose(prep);

  // TODO: mettere dist_create con "master" e "Inizialization"

  // char dist_path[256];
  // snprintf(dist_path, sizeof(dist_path), ".svm/dists/%s", "master");
  // FILE *dist_f = fopen(dist_path, "w");
  // if (!dist_f)
  //   return false;
  //
  //
  // fprintf(dist_f, "%s\n", "CIOLA");
  // fclose(dist_f);

  return true;
}
