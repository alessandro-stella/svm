#ifndef BLOB_HANDLER_H
#define BLOB_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

// Funzione generica per creare un oggetto (usata internamente da add.c per i tree)
char *create_object_from_data(const char *type, const unsigned char *content_data, size_t content_size, size_t *out_full_size);

// Metodo 1: Creazione Blob leggendo da File (path)
char *create_blob_from_file(const char *filename, size_t *blob_size_out);

// Metodo 2: Creazione Blob leggendo da Memoria (char array)
char *create_blob_from_memory(const char *data, size_t data_size);

// Funzione di I/O (salva i dati compressi con le dimensioni corrette)
bool create_file(const char *hex_hash, const unsigned char *compressed_content, size_t compressed_size, size_t original_uncompressed_size);

#endif
