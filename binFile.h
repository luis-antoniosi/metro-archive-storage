#ifndef BINFILE_H
#define BINFILE_H

#include <stdio.h>
#include "types.h"
#include "register.h"

// Header
Header *create_header();
int write_header(FILE *file, Header *header);

void status0(FILE *file);
void status1(FILE *file);

// Write file
DataStatus write_bin_file(FILE *inputFile, FILE *outputFile);

// Print
DataStatus print_all_data(FILE *binFile);
DataStatus print_all_data_where(FILE *binFile, int iterations);

// Delete
DataStatus delete_all_data_where(FILE *binFile, int iterations);

// Print binary
void binary_on_screen(char *fileName);

#endif