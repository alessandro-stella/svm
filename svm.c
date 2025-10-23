#include "svm_commands.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void print_help() {
  const char *commands[8][3] = {{"help", "", "Display all svm commands"},
                                {"init", "", "Initialize svm project inside current directory"},
                                {"clean", "", "Remove svm project inside current directory"},
                                {"add", "", "Add all files in current directory to current svm distribution"},
                                {"unpack", "<blob_hash>", "Show content of a blob of current svm distribution"},
                                {"dist", "", "Show add distributions in current project"},
                                {"dist create", "<dist_name>", "Create a new distribution"},
                                {"switch", "<dist_name>", "Switch to another distribution"}

  };

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
    // Help
  case 'h': {
    if (strcmp(argv[1], "help") != 0) {
      printf("Unknown command!\n\n");
    }

    print_help();
  } break;

    // Init
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
      printf("Error during initialization\n");
      return -1;
    }

    printf("svm initialized successfully\n");
  } break;

    // Add
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

    // TODO: Chiamo add_command, genero l'hash del tree che punta allo stato attuale della dist
    // Salva dentro dists/... l'hash generato, il messaggio e il tree precedente (capire come)
  } break;

    // Unpack
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

    free(decompressed);
  } break;

    // Clean
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

    // Dist
  case 'd': {
    if (strcmp(argv[1], "dist") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (argc == 2) {
      dist_show();
      break;
    }

    if (argc != 4 || strcmp(argv[2], "create") != 0) {
      printf("Syntax error!\n");
      printf("git dist create <diff_name>\n");
      break;
    }

    const char *dist_path = ".svm/dists";
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", dist_path, argv[3]);

    FILE *fd = fopen(path, "r");
    if (fd) {
      printf("Dist \"%s\" already exists\n", argv[3]);
      fclose(fd);
      break;
    }

    printf("Are you sure you want to create a dist called \"%s\"? [y/N]: ", argv[3]);
    fflush(stdin);
    char choice;
    scanf("%c", &choice);

    if (choice != 'y' && choice != 'Y') {
      break;
    }

    if (dist_create(argv[3], path)) {
      printf("\nDist \"%s\" created successfully\n", argv[3]);
    }
  } break;

    // Switch
  case 's': {
    if (strcmp(argv[1], "switch") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (argc != 3) {
      printf("Syntax error!\n");
      printf("git switch <diff_name>\n");
      break;
    }

    const char *dist_path = ".svm/dists";
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", dist_path, argv[2]);

    FILE *fd = fopen(path, "r");
    if (!fd) {
      printf("Dist \"%s\" does not exists\n", argv[2]);
      break;
    }

    fclose(fd);

    if (switch_command(argv[2], path)) {
      printf("Switched to dist \"%s\"\n", argv[2]);
    }
  } break;

    // Prepare
  case 'p': {
    if (strcmp(argv[1], "prepare") != 0 && strcmp(argv[1], "prep")) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (argc != 3) {
      printf("Wrong syntax\n");
      break;
    }

    if (strcmp(argv[2], ".") == 0) {
      if (prepare_all(".") == 'e') {
        printf("Error while preparing project for dist\n");
      }

      return -1;
    }

    char res = prepare_file(argv[2]);

    switch (res) {
    case 'a': {
      printf("Added %s to tracking\n", argv[2]);
    } break;

    case 'u': {
      printf("Updated %s to current tracking\n", argv[2]);
    } break;

    default: {
      printf("Error while preparing file for dist\n");
      return -1;
    } break;
    }
  } break;

  default:
    printf("Unknown command!\n\n");
    print_help();
    break;
  }

  return 1;
}
