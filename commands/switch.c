#include "../svm_commands.h"
#include "../utils/constants.h"
#include "../utils/restore.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void split_line3(char *input_line, char **type, char **hash, char **name) {
  if (input_line == NULL || *input_line == '\0') {
    *type = *hash = *name = NULL;
    return;
  }

  const char *delimiter = " ";
  char *context;

  *type = strtok_r(input_line, (char *)delimiter, &context);
  *hash = strtok_r(NULL, (char *)delimiter, &context);
  *name = strtok_r(NULL, (char *)delimiter, &context);

  if (*name != NULL)
    (*name)[strcspn(*name, "\n\r")] = '\0';
}

void restore_tree(char *tree, char *prev_path) {
  if (mkdir(prev_path, 0755) == 0) {
    printf("Created folder: %s\n", prev_path);
  } else {
    if (errno != EEXIST) {
      perror("Error creating folder");
      return;
    }
  }

  if (tree == NULL || *tree == '\0') {
    return;
  }

  char *tree_copy = strdup(tree);
  if (tree_copy == NULL) {
    perror("strdup failed");
    return;
  }

  char *context_line;
  char *line;
  int line_num = 1;

  char *delimiter_line = "\n";

  line = strtok_r(tree_copy, delimiter_line, &context_line);

  while (line != NULL && strlen(line) > 4) {
    char *type, *hash, *name;

    split_line3(line, &type, &hash, &name);

    if (type == NULL) {
      printf("Parsing error on line of %s tree\n", prev_path);
    }

    size_t name_len = strlen(name);
    bool name_has_trailing_slash = (name_len > 0 && name[name_len - 1] == '/');

    size_t clean_name_len = name_has_trailing_slash ? name_len - 1 : name_len;

    size_t len = strlen(prev_path) + 1 + clean_name_len + 1;

    char *new_path = (char *)malloc(len);
    if (new_path == NULL) {
      perror("malloc failed");
      line = strtok_r(NULL, delimiter_line, &context_line);
      line_num++;
      continue;
    }

    char temp_name[name_len + 1];
    strcpy(temp_name, name);
    if (name_has_trailing_slash) {
      temp_name[name_len - 1] = '\0';
    }

    snprintf(new_path, len, "%s/%s", prev_path, temp_name);

    if (strcmp(type, "tree") == 0) {
      char *tree_content = get_tree(hash);

      if (tree_content == NULL) {
        printf("Couln't get distribution tree for %s\n", new_path);
      } else {
        restore_tree(tree_content, new_path);
        free(tree_content);
      }
    } else {
      if (restore_file_from_blob(hash, new_path)) {
        printf("File %s restored successfully\n", new_path);
      } else {
        printf("Error while restoring %s\n", new_path);
      }
    }

    free(new_path);

    line = strtok_r(NULL, delimiter_line, &context_line);
    line_num++;
  }

  free(tree_copy);
}

bool restore_dist(char *tree_hash, char *path) {
  if (clear_command(path) == -1) {
    printf("Error during clear command\n");
    return false;
  }

  char *tree = get_tree(tree_hash);
  if (tree == NULL) {
    printf("Couln't get distribution tree\n");
    return false;
  }

  restore_tree(tree, ".");
  free(tree);
  return true;
}

bool get_latest_tree_hash(const char *dist_name, char *tree_hash, size_t hash_buffer_size) {
  char dist_path[MAX_PATH_LEN];

  if (snprintf(dist_path, sizeof(dist_path), ".svm/dists/%s", dist_name) >= sizeof(dist_path)) {
    fprintf(stderr, "Error: Distribution path too long.\n");
    return false;
  }

  FILE *fd = fopen(dist_path, "r");
  if (fd == NULL) {
    perror("Error opening distribution file");
    return false;
  }

  char last_line[BUF_SIZE] = {0};
  char current_line[BUF_SIZE];

  while (fgets(current_line, sizeof(current_line), fd) != NULL) {
    if (strlen(current_line) >= BUF_SIZE - 1 && current_line[BUF_SIZE - 2] != '\n') {
      int c;
      while ((c = fgetc(fd)) != '\n' && c != EOF)
        ;
      continue;
    }

    strcpy(last_line, current_line);
  }

  fclose(fd);

  if (strlen(last_line) == 0) {
    fprintf(stderr, "Distribution file is empty or corrupted: %s\n", dist_path);
    return false;
  }

  char *hash_start = strtok(last_line, " \t\n\r");

  if (hash_start == NULL || strlen(hash_start) != SHA256_HEX_LEN) {
    fprintf(stderr, "Failed to parse valid tree hash from the last line of %s\n", dist_path);
    return false;
  }

  if (hash_buffer_size < SHA256_HEX_LEN + 1) {
    fprintf(stderr, "Tree hash buffer too small.\n");
    return false;
  }

  strcpy(tree_hash, hash_start);

  return true;
}

bool switch_command(const char *dist_name, const char *path) {
  FILE *c_d_read = fopen(".svm/current_dist", "r");
  char current_dist_name[MAX_DIST_NAME_LEN];
  bool is_current = false;

  if (c_d_read != NULL) {
    if (fgets(current_dist_name, sizeof(current_dist_name), c_d_read) != NULL) {
      size_t len = strlen(current_dist_name);
      if (len > 0 && current_dist_name[len - 1] == '\n') {
        current_dist_name[len - 1] = '\0';
      }

      if (strcmp(current_dist_name, dist_name) == 0) {
        is_current = true;
      }
    }
    fclose(c_d_read);
  }

  if (is_current) {
    printf("Already on dist \"%s\"\n", dist_name);
    return true;
  }

  char latest_tree_hash[SHA256_HEX_LEN + 1];

  if (!get_latest_tree_hash(dist_name, latest_tree_hash, sizeof(latest_tree_hash))) {
    fprintf(stderr, "Error: Could not determine the latest tree hash for dist \"%s\".\n", dist_name);
    return false;
  }

  FILE *fd = fopen(path, "r");
  FILE *c_d_write = fopen(".svm/current_dist", "w");

  if (fd == NULL || c_d_write == NULL) {
    printf("Error during dist switch: could not open files for writing.\n");

    if (fd)
      fclose(fd);
    if (c_d_write)
      fclose(c_d_write);
    return false;
  }

  fprintf(c_d_write, "%s", dist_name);

  if (!restore_dist(latest_tree_hash, ".")) {
    printf("Couldn't restore dist\n");
    return false;
  }

  if (fclose(fd) != 0 || fclose(c_d_write) != 0) {
    return false;
  }

  printf("Switched to dist \"%s\"\n", dist_name);
  return true;
}
