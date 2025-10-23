#include "../svm_commands.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool switch_command(const char *dist_name, const char *path) {
  FILE *fd = fopen(path, "w");
  FILE *c_d = fopen(".svm/current_dist", "w");

  if (fd == NULL || c_d == NULL) {
    printf("Error during dist switch\n");
    return false;
  }

  fprintf(c_d, "%s", dist_name);
  printf("IMPLEMENTARE LOGICA DI RICOSTRUZIONE DATI\n");

  if (fclose(fd) == -1 || fclose(c_d)) {
    return false;
  }

  return true;
}
