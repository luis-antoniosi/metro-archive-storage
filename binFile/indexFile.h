#ifndef INDEX_FILE_H
#define INDEX_FILE_H

#include <stdio.h>
#include "types.h"

// part 2
// TODO: Add comments for these functions; maybe make them be in another file indexFile.c/.h
Status create_index(FILE *registerFile, FILE *indexFile);
Status search_with_index(FILE *registerFile, FILE *indexFile, int iterations);
Status insert_index(FILE *registerFile, FILE *indexFile, int iterations);
Status delete_index(FILE *registerFile, FILE *indexFile, int iterations);

#endif