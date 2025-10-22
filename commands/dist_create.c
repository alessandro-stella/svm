#include "../svm_commands.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool dist_create(const char *dist_name, const char *path) {
  FILE *f = fopen(path, "w");
  FILE *c_d = fopen(".svm/current_dist", "w");

  if (f == NULL || c_d == NULL) {
    printf("Error during dist creation\n");
    return false;
  }

  fprintf(c_d, "%s", dist_name);

  char *tree_hash = add_command(".");
  fprintf(f, "%s", tree_hash);

  if (fclose(f) == -1 || fclose(c_d) == -1) {
    printf("Error during dist creation\n");
    return false;
  }

  return true;
}
