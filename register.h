#ifndef REGISTER_H
#define REGISTER_H

#include <stdio.h>
#include "types.h"

Data *tokenize(char *buffer);

void write_data(FILE *binFile, Data *data);
Data *read_data(FILE *binFile);
void remove_data(FILE *binFile);

SearchField *get_all_search_fields(int *pairIterations);
Data *check_data_field_search(FILE *binFile, SearchField *filters, int pairIterations, int *isMatch);

void print_data(Data *data);

void destroy_data(Data **data); 

#endif