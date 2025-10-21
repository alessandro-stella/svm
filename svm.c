#include "svm_commands.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void print_help() {
  const char *commands[5][3] = {{"help", "", "Display all svm commands"},
                                {"init", "", "Initialize svm project inside current directory"},
                                {"clean", "", "Remove svm project inside current directory"},
                                {"add", "", "Add all files in current directory to current svm distribution"},
                                {"unpack", "<blob_hash>", "Show content of a blob of current svm distribution"}};

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
  if (stat(".svm", &st) == -1 && strcmp(argv[1], "init")) {
    printf("svm not initialized\n");
    return -1;
  }

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

    if (!init_command()) {
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

    if (stat(".svm/dists", &st) == -1) {
      printf("Distribution directory missing\n");
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

    if (file_size <= 0) {
      printf("Error while retreiving current dist (empty file)");
      fclose(f);
      return -1;
    }

    char *current_dist = (char *)malloc(file_size + 1);
    if (!current_dist) {
      perror("malloc");
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

    char *tree_hash = add_command(".");

    if (tree_hash == NULL) {
      printf("\nError during tree creation\n");
    } else {
      printf("\nTree generated successfully\n");
      printf("Hash: %s\n", tree_hash);

      // OPTIONAL: Se vuoi salvare l'hash nel file della distribuzione
      // (cosa che farebbe un commit), puoi riaprire il file qui in append.
      /*
      char dist_path[256];
      snprintf(dist_path, sizeof(dist_path), ".svm/dists/%s", current_dist);
      FILE *dist_f = fopen(dist_path, "a");
      if (dist_f) {
          fprintf(dist_f, "%s\n", tree_hash);
          fclose(dist_f);
      }
      */
    }

    free(tree_hash);
    free(current_dist);
  } break;

  case 'u': {
    if (strcmp(argv[1], "unpack") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    struct stat st;

    if (stat(".svm/objects", &st) == -1) {
      printf("objects directory missing\n");
      return -1;
    }

    if (argc != 3) {
      printf("Not enough arguments!\n");
      printf("Usage: svm unpack <blob>\n");
      return -1;
    }

    char subdir[256];
    snprintf(subdir, sizeof(subdir), ".svm/objects/%.2s", argv[2]);
    if (stat(subdir, &st) == -1) {
      printf("Blob not found\n");
      return -1;
    }

    size_t original_len;
    char *decompressed = unpack_command(argv[2], &original_len);

    if (!decompressed) {
      printf("Failed to decompress blob.\n");
      return -1;
    }

    char *content_start = strchr(decompressed, '\0');

    if (content_start == NULL) {
      free(decompressed);
      return -1;
    }

    content_start++;

    size_t content_len = original_len - (content_start - decompressed);

    fwrite(content_start, 1, content_len, stdout);

    printf("\n");

    free(decompressed);
  } break;

  case 'c': {
    if (strcmp(argv[1], "clean") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    printf("Are you sure you want to remove svm from current project? [y/N]: ");
    char choice;
    fflush(stdin);
    scanf(" %c", &choice);

    if (choice != 'y' && choice != 'Y')
      return 0;

    if (clear_command(".svm") != 0) {
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
