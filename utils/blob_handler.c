#include "blob_handler.h"
#include "hashing.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

// ===================================================
// Creation of file
// ===================================================

// TODO: Creare funzione per:
// 1) Creare un blob compresso a partire da dei dati
// 2) Decomprime un blob dinamicamente
