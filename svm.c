#include "blob_handler.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define HASH_SIZE 64;

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("No arguments!\n");
    return 1;
  }

  struct stat st;

  switch (argv[1][0]) {

  case 'i': {
    if (strcmp(argv[1], "init") != 0) {
      printf("BOH 2");
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
      printf("Errore nella scrittura del contenuto");
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
      printf("BOH 1");
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

    DIR *d;
    struct dirent *dir;

    d = opendir(".");

    if (d) {
      while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
          if (dir->d_name[0] == '.') {
            continue;
          }
        }

        size_t file_size;
        char *hash = create_blob(dir->d_name, &file_size);

        if (hash == NULL) {
          printf("Error during blob creation of %s\n", dir->d_name);
          continue;
        }

        printf("Blob for %s (original size: %lu) created: %s", dir->d_name, file_size, hash);
        printf("\n");

        fprintf(f, "blob %s %lu %s\n", hash, file_size, dir->d_name);
        free(hash);
      }
      closedir(d);
    }

    fclose(f);
  } break;

  case 'c': {
    if (strcmp(argv[1], "create") != 0) {
      printf("BOH 1");
      break;
    }

    if (argc != 3) {
      printf("Not enough arguments!\n");
      printf("Usage: svm create <filename>\n");
      break;
    }

    size_t file_size;
    char *hash = create_blob(argv[2], &file_size);

    if (hash == NULL) {
      printf("Error during blob creation\n");
      break;
    }

    printf("Hash: %s - File size: %lu", hash, file_size);

    free(hash);
  } break;

  case 'u': {
    if (strcmp(argv[1], "unpack") != 0) {
      printf("BOH 3");
      break;
    }

    if (stat(".svm", &st) == -1) {
      printf("svm not initialized\n");
      return false;
    }
    if (stat(".svm/objects", &st) == -1) {
      printf("objects directory missing\n");
      return false;
    }

    if (argc != 4) {
      printf("Not enough arguments!\n");
      printf("Usage: svm unpack <blob> <original_size>\n");
      break;
    }

    char subdir[256];
    snprintf(subdir, sizeof(subdir), ".svm/objects/%.2s", argv[2]);
    if (stat(subdir, &st) == -1) {
      printf("Blob not found\n");
      break;
    }

    size_t original_len = strtoul(argv[3], NULL, 10);
    unsigned char *decompressed = read_blob(argv[2], original_len);

    if (!decompressed) {
      printf("Failed to decompress blob.\n");
      break;
    }

    fwrite(decompressed, 1, original_len, stdout);
    free(decompressed);
  } break;

  default:
    printf("BOH");
    break;
  }

  return 1;
}
