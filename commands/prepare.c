#include "../svm_commands.h"
#include "../utils/hashing.h"

#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char prepare_file(const char *path) {
  if (strncmp(path, "./", 2) != 0) {
    printf("Invalid file path\n");
    return 'e';
  }

  FILE *fd = fopen(path, "rb");
  if (!fd) {
    printf("File not found\n");
    return 'e';
  }

  FILE *prep = fopen(".svm/prep", "r");
  if (!prep) {
    printf("Unable to find \"prep\" file inside .svm\n");
    fclose(fd);
    return 'e';
  }

  FILE *prep_tmp = fopen(".svm/prep_tmp", "w");
  if (!prep_tmp) {
    printf("Unable to create \"prep\" file inside .svm\n");
    fclose(fd);
    fclose(prep);
    return 'e';
  }

  fseek(fd, 0, SEEK_END);
  size_t size = ftell(fd);
  rewind(fd);

  unsigned char *buffer = malloc(size);
  if (!buffer) {
    printf("Error during memory allocation\n");
    fclose(fd);
    fclose(prep);
    fclose(prep_tmp);
    return 'e';
  }

  fread(buffer, 1, size, fd);
  fclose(fd);

  size_t hash_len;
  const unsigned char *hash = create_hash(buffer, size, &hash_len);
  const char *hash_hex = hash_to_hex(hash, hash_len);
  free(buffer);

  size_t bufsize = 128;
  char *line = malloc(bufsize);
  if (!line) {
    printf("Error during memory allocation\n");
    fclose(prep);
    fclose(prep_tmp);
    return 'e';
  }

  bool found = false;

  while (fgets(line, bufsize, prep)) {
    size_t len = strlen(line);

    while (len > 0 && line[len - 1] != '\n' && !feof(prep)) {
      bufsize *= 2;
      char *tmp = realloc(line, bufsize);
      if (!tmp) {
        printf("Error during memory allocation\n");
        free(line);
        fclose(prep);
        fclose(prep_tmp);
        return 'e';
      }
      line = tmp;

      if (fgets(line + len, bufsize - len, prep))
        len = strlen(line);
      else
        break;
    }

    if (len > 0 && line[len - 1] == '\n')
      line[len - 1] = '\0';

    char *last_space = strrchr(line, ' ');
    if (last_space)
      last_space++;

    if (last_space && strcmp(path, last_space) == 0) {
      fprintf(prep_tmp, "%s %s\n", hash_hex, path);
      found = true;
    } else {
      fprintf(prep_tmp, "%s\n", line);
    }
  }

  char result = found ? 'u' : 'a';
  if (!found) {
    fprintf(prep_tmp, "%s %s\n", hash_hex, path);
  }

  free(line);
  fclose(prep);
  fclose(prep_tmp);

  remove(".svm/prep");
  rename(".svm/prep_tmp", ".svm/prep");

  return result;
}

char prepare_all(const char *path) {
  DIR *d = opendir(path);
  if (!d)
    return 'e';

  struct dirent *dir;
  char overall_result = 'a';

  while ((dir = readdir(d)) != NULL) {
    if (dir->d_name[0] == '.')
      continue;
    if (dir->d_type != DT_DIR && dir->d_type != DT_REG)
      continue;

    size_t full_path_size = strlen(path) + strlen(dir->d_name) + 2;
    char *full_path = malloc(full_path_size);
    if (!full_path) {
      printf("Error during memory allocation\n");
      closedir(d);
      return 'e';
    }
    snprintf(full_path, full_path_size, "%s/%s", path, dir->d_name);

    char result;
    if (dir->d_type == DT_DIR) {
      result = prepare_all(full_path);
    } else {
      result = prepare_file(full_path);
    }

    if (result == 'e') {
      free(full_path);
      closedir(d);
      return 'e';
    } else if (result == 'a' || result == 'u') {
      overall_result = 'a';
      if (result == 'a')
        printf("Added %s to tracking\n", full_path);
      else
        printf("Updated %s to current tracking\n", full_path);
    }

    free(full_path);
  }

  closedir(d);
  return overall_result;
}
