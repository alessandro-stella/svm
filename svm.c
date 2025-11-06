#include "svm_commands.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void print_help() {
  const char *commands[9][3] = {{"help", "", "Display all svm commands"},
                                {"init", "", "Initialize svm project inside current directory"},
                                {"clean", "", "Remove svm project inside current directory"},
                                {"add", "", "Add all files prepared with \"prep\" to current svm distribution"},
                                {"prep", "", "Prepare files before adding to distribution"},
                                {"unpack", "<blob_hash>", "Show content of a blob of current svm distribution"},
                                {"dist", "", "Show add distributions in current project"},
                                {"dist create", "<dist_name>", "Create a new distribution"},
                                {"switch", "<dist_name>", "Switch to another distribution"}};

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
  // Check for .svm folder unless the command is "init"
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
    return 0;
  }

  // Init
  case 'i': {
    if (strcmp(argv[1], "init") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (stat(".svm", &st) != -1) {
      printf("svm already initialized\n");
      return -1;
    }

    if (!init_command()) {
      printf("Error during initialization\n");
      return -1;
    }

    printf("svm initialized successfully\n");
    return 0;
  }

  // Add
  case 'a': {
    if (strcmp(argv[1], "add") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (stat(".svm/dists", &st) == -1) {
      printf("Distribution directory missing\n");
      return -1;
    }

    char *res = add_command();

    if (res == NULL) {
      printf("Error during add command\n");
      return -1;
    }

    printf("Distribution tree created successfully!\nHash: %s\n", res);
  } break;

    // Unpack
  case 'u': {
    if (strcmp(argv[1], "unpack") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    struct stat st_unpack;
    if (stat(".svm/objects", &st_unpack) == -1) {
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

    char blob_path[512];
    snprintf(blob_path, sizeof(blob_path), "%s/%s", subdir, argv[2] + 2);

    if (stat(blob_path, &st_unpack) == -1) {
      printf("Blob not found (hash: %s)\n", argv[2]);
      return -1;
    }

    size_t original_len;
    char *decompressed = unpack_command(argv[2], &original_len);

    if (!decompressed) {
      printf("Failed to decompress blob.\n");
      return -1;
    }

    // --- LOGICA DI PARSING DELL'HEADER CORRETTA ---

    // 1. Controlla che inizi con "blob "
    if (strncmp(decompressed, "blob ", 5) != 0) {
      fprintf(stderr, "Header decompresso malformato (non inizia con 'blob ').\n");
      free(decompressed);
      return -1;
    }

    // 2. Trova il byte nullo ('\0') che separa l'header dalla parte binaria.
    // Iniziamo la ricerca dopo "blob " (5 caratteri) e cerchiamo fino alla fine.
    // Si usa memchr perché il byte nullo è un separatore, non un terminatore di stringa qui.
    char *header_end = (char *)memchr(decompressed + 5, '\0', original_len - 5);

    if (header_end == NULL) {
      fprintf(stderr, "Header decompresso malformato (manca byte nullo separatore).\n");
      free(decompressed);
      return -1;
    }

    // L'inizio del contenuto è SUBITO DOPO il byte nullo ('\0')
    char *content_start = header_end + 1;

    // --- FINE LOGICA DI PARSING ---

    size_t content_len = original_len - (content_start - decompressed);

    fwrite(content_start, 1, content_len, stdout);

    free(decompressed);
    return 0;
  }
    // Clean
  case 'c': {
    if (strcmp(argv[1], "clean") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    printf("Are you sure you want to remove svm from current project? [y/N]: ");
    char choice;
    scanf(" %c", &choice);

    if (choice != 'y' && choice != 'Y')
      return 0;

    if (clear_command(".svm") != 0) {
      perror("Couldn't remove svm from current project");
      return -1;
    }

    printf("svm removed successfully\n");
    return 0;
  }

  // Dist
  case 'd': {
    if (strcmp(argv[1], "dist") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (argc == 2) {
      dist_show();
      return 0;
      break;
    }

    if (argc != 4 || strcmp(argv[2], "create") != 0) {
      printf("Syntax error!\n");
      printf("git dist create <diff_name>\n");
      return -1;
    }

    const char *dist_path = ".svm/dists";
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", dist_path, argv[3]);

    FILE *fd = fopen(path, "r");
    if (fd) {
      printf("Dist \"%s\" already exists\n", argv[3]);
      fclose(fd);
      return -1;
    }

    printf("Are you sure you want to create a dist called \"%s\"? [y/N]: ", argv[3]);
    char choice;
    scanf(" %c", &choice);

    if (choice != 'y' && choice != 'Y') {
      return 0;
    }

    if (dist_create(argv[3], path)) {
      printf("\nDist \"%s\" created successfully\n", argv[3]);
      return 0;
    }
    return -1;
  }

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
      return -1;
    }

    const char *dist_path = ".svm/dists";
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", dist_path, argv[2]);

    FILE *fd = fopen(path, "r");
    if (!fd) {
      printf("Dist \"%s\" does not exists\n", argv[2]);
      return -1;
    }

    fclose(fd);

    if (switch_command(argv[2], path)) {
      printf("Switched to dist \"%s\"\n", argv[2]);
      return 0;
    }
    return -1;
  }

  // Prepare
  case 'p': {
    if (strcmp(argv[1], "prep") != 0) {
      printf("Unknown command!\n\n");
      print_help();
      return -1;
    }

    if (!prep_command(".")) {
      printf("Error while preparing project for dist\n");
      return -1;
    }

    printf("Project prepared successfully.\n");
    return 0;
  }

  default:
    printf("Unknown command!\n\n");
    print_help();
    break;
  }

  return 1;
}
