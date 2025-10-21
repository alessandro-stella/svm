#include "blob_handler.h"
#include "directory_handler.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void print_help() {
  const char *commands[5][3] = {{"help", "", "Display all svm commands"},
                                {"init", "", "Initialize svm project inside current directory"},
                                {"clean", "", "Remove svm project inside current directory"},
                                {"add", "", "Add all files in current directory to current svm distribution"},
                                {"unpack", "<blob_name>", "Show content of blob of current svm distribution"}};

  for (int i = 0; i < 5; i++) {
    printf("svm %s %s\n", commands[i][0], commands[i][1]);
    printf("%s\n\n", commands[i][2]);
  }
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    print_help();
    return 1;
  }

  struct stat st;

  switch (argv[1][0]) {
  case 'h': {
    if (strcmp(argv[1], "help") != 0) {
      printf("Unknown command!\n\n");
    }

    print_help();
  } break;

  case 'i': {
    if (strcmp(argv[1], "init") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      break;
    }

    if (stat(".svm", &st) != -1) {
      printf("svm already initialized\n");
      break;
    }

    mkdir(".svm", 0700);
    mkdir(".svm/objects", 0700);
    mkdir(".svm/dists", 0700);

    FILE *current_dist = fopen(".svm/current_dist", "wb");

    size_t content_len = strlen("master");
    if (fwrite("master", 1, content_len, current_dist) != content_len) {
      printf("Error while writing current dist");
      fclose(current_dist);
      return -1;
    }

    if (current_dist == NULL) {
      printf("Error during initialization");
      return -1;
    }

    if (fclose(current_dist) != 0) {
      printf("Error during initialization");
      return -1;
    }

    printf("svm initialized successfully\n");
  } break;

  case 'a': {
    if (strcmp(argv[1], "add") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      break;
    }

    if (stat(".svm", &st) == -1) {
      printf("svm not initialized\n");
      break;
    }
    if (stat(".svm/dists", &st) == -1) {
      printf("distribution directory missing\n");
      break;
    }

    FILE *f = fopen(".svm/current_dist", "rb");
    if (!f) {
      printf("Error while retreiving current dist");
      return -1;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0) {
      printf("Error while retreiving current dist");
      fclose(f);
      return -1;
    }

    char *current_dist = (char *)malloc(file_size + 1);
    if (!current_dist) {
      printf("Error during memory allocation");
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

    char dist_path[256];
    snprintf(dist_path, sizeof(dist_path), ".svm/dists/%s", current_dist);

    f = fopen(dist_path, "w");

    if (!f) {
      printf("Error while opening or creating dist file");
      return -1;
    }

    size_t tree_size;
    char *tree_hash = create_tree(".", &tree_size);

    if (tree_hash == NULL) {
      printf("\nError during tree creation\n");
    } else {
      printf("\nTree generated successfully\n");
      printf("Hash: %s - size: %lu", tree_hash, tree_size);
    }

    free(tree_hash);
    fclose(f);
  } break;

  case 'u': {
    if (strcmp(argv[1], "unpack") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (stat(".svm", &st) == -1) {
      printf("svm not initialized\n");
      return -1;
    }
    if (stat(".svm/objects", &st) == -1) {
      printf("objects directory missing\n");
      return -1;
    }

    if (argc != 4) {
      printf("Not enough arguments!\n");
      printf("Usage: svm unpack <blob> <original_size>\n");
      return -1;
    }

    char subdir[256];
    snprintf(subdir, sizeof(subdir), ".svm/objects/%.2s", argv[2]);
    if (stat(subdir, &st) == -1) {
      printf("Blob not found\n");
      return -1;
    }

    size_t original_len = strtoul(argv[3], NULL, 10);
    unsigned char *decompressed = read_blob(argv[2], original_len);

    if (!decompressed) {
      printf("Failed to decompress blob.\n");
      return -1;
    }

    fwrite(decompressed, 1, original_len, stdout);
    free(decompressed);
  } break;

  case 'c': {
    if (strcmp(argv[1], "clean") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (stat(".svm", &st) == -1) {
      printf("svm not initialized\n");
      return -1;
    }

    printf("Are you sure you want to remove svm from current project? [y/N]: ");
    char choice;
    scanf("%c", &choice);

    if (choice != 'y' && choice != 'Y')
      return 0;

    if (remove_dir(".svm") != 0) {
      perror("Couldn't remove svm from current project");
      return -1;
    }

    printf("svm removed successfully\n");
  } break;

  default:
    printf("Unknown command!\n\n");
    print_help();
    break;
  }

  return 1;
}
