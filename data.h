#ifndef DATA_H
#define DATA_H

#include <stdio.h>

#define BUF_SIZE 256
#define DATA_SIZE 80
#define TRASH '$'

typedef enum DataStatus
{
    DATA_SUCCESS = 0,
    DATA_FAILURE = 1
} DataStatus;

typedef struct Data
{
    char removed;
    int next;

    int stationCode;
    int lineCode;

    int nextStationCode;
    int distNextStation;

    int codeIntegLine;
    int codeIntegStation;

    int sizeStationName;
    char *stationName;

    int sizeLineName;
    char *lineName;
} Data;

Data *tokenize(char *buffer);

void write_data(FILE *binFile, Data *data);
int write_bin_file(FILE *inputFile, FILE *outputFile);

int print_all_data(FILE *binFile);

void destroy_data(Data **data);

#endif