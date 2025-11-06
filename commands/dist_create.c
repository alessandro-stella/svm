#include "../svm_commands.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool dist_create(const char *dist_name, const char *path) {
  FILE *fd = fopen(path, "w");
  FILE *c_d = fopen(".svm/current_dist", "w");

  if (fd == NULL || c_d == NULL) {
    printf("Error during dist creation\n");
    return false;
  }

  fprintf(c_d, "%s", dist_name);

  bool prep_result = prep_command(".");

  if (!prep_result) {
    printf("Error during preparation of dist");
    return false;
  }

  char *tree_hash = add_command("Initialization");
  if (tree_hash == NULL) {
    printf("Error while adding prepared files to dist");
    return false;
  }

  fprintf(fd, "%s", tree_hash);

  if (fclose(fd) == -1 || fclose(c_d) == -1) {
    printf("Error during dist creation\n");
    return false;
  }

  return true;
}
