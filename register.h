#ifndef REGISTER_H
#define REGISTER_H

#include <stdio.h>
#include "types.h"

Register *parse_register(char *buffer);

void write_register(FILE *binFile, Register *data);
Register *read_register(FILE *binFile);
void remove_register(FILE *binFile);

SearchField *get_all_search_fields(int *pairIterations);
Register *check_register_field_search(FILE *binFile, SearchField *filters, int pairIterations, int *isMatch);

void print_register(Register *Register);

void destroy_register(Register **Register); 

#endif