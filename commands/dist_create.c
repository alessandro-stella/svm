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

  // TODO: Fare prima prep
  char *tree_hash = add_command();
  fprintf(fd, "%s", tree_hash);

  if (fclose(fd) == -1 || fclose(c_d) == -1) {
    printf("Error during dist creation\n");
    return false;
  }

  return true;
}
