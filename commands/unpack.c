#include "../svm_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

char *unpack_command(const char *hex_hash, size_t *out_original_len) {
  // TODO: Creare funzione che prende un hex_hash, chiama la funzione da blob_handler,
  // spacchetta i dati e restituisce i dati leggibili all'utente

  return "CIOLA";
}

unsigned char *decompress_file(const unsigned char *blob, size_t blob_size, size_t original_len) {
  unsigned char *decompressed = malloc(original_len);
  if (!decompressed)
    return NULL;

  uLongf dest_len = (uLongf)original_len;

  if (uncompress(decompressed, &dest_len, blob, blob_size) != Z_OK) {
    printf("Decompression failed\n");
    free(decompressed);
    return NULL;
  }

  return decompressed;
}
