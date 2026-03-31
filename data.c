#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "data.h"

void write_data(FILE *binFile, Data *data);
int check_for_null(char *str);

Data *read_data(FILE *binFile);
void print_data(Data *data);

void print_int_or_null(int value)
{
    if (value == -1)
        printf("NULO");
    else
        printf("%d", value);
}

void print_str_or_null(int size, char *str)
{
    if (size == 0 || str == NULL || str[0] == '\0')
        printf("NULO");
    else
        printf("%s", str);
}

int check_for_null(char *str)
{
    return !str || strcspn(str, "\n") == 0 ? -1 : atoi(str);
}

Data *tokenize(char *buffer)
{
    Data *tempData = malloc(sizeof(Data));

    char *context;

    char *token = strtok_s(buffer, ",", &context);
    tempData->stationCode = check_for_null(token);

    token = strtok_s(NULL, ",", &context);
    if (token)
    {
        int len = strlen(token);
        tempData->stationName = malloc(sizeof(char) * len);
        tempData->stationName = strdup(token);
        tempData->sizeStationName = len;
    }

    token = strtok_s(NULL, ",", &context);
    tempData->lineCode = check_for_null(token);

    token = strtok_s(NULL, ",", &context);
    if (token)
    {
        int len = strlen(token);
        tempData->lineName = malloc(sizeof(char) * len);
        tempData->lineName = strdup(token);
        tempData->sizeLineName = len;
    }

    token = strtok_s(NULL, ",", &context);
    tempData->nextStationCode = check_for_null(token);

    token = strtok_s(NULL, ",", &context);
    tempData->distNextStation = check_for_null(token);

    token = strtok_s(NULL, ",", &context);
    tempData->codeIntegLine = check_for_null(token);

    token = strtok_s(NULL, ",", &context);
    tempData->codeIntegStation = check_for_null(token);
    
    return tempData;
}

void write_data(FILE *binFile, Data *data)
{
    long start = ftell(binFile);

    fwrite(&data->removed, sizeof(char), 1, binFile);
    fwrite(&data->next, sizeof(int), 1, binFile);

    fwrite(&data->stationCode, sizeof(int), 1, binFile);
    fwrite(&data->lineCode, sizeof(int), 1, binFile);

    fwrite(&data->nextStationCode, sizeof(int), 1, binFile);
    fwrite(&data->distNextStation, sizeof(int), 1, binFile);

    fwrite(&data->codeIntegLine, sizeof(int), 1, binFile);
    fwrite(&data->codeIntegStation, sizeof(int), 1, binFile);

    fwrite(&data->sizeStationName, sizeof(int), 1, binFile);
    if (data->sizeStationName > 0)
        fwrite(data->stationName, data->sizeStationName, 1, binFile);

    fwrite(&data->sizeLineName, sizeof(int), 1, binFile);
    if (data->sizeLineName > 0)
        fwrite(data->lineName, data->sizeLineName, 1, binFile);

    long end = ftell(binFile);
    int newDataSize = end - start;

    int remainingBytes = DATA_SIZE - newDataSize;
    if (remainingBytes > 0)
    {
        char tmp = TRASH;
        for (int i = 0; i < remainingBytes; i++)
        {
            fwrite(&tmp, sizeof(tmp), 1, binFile);
        }
    }
}

int write_bin_file(FILE *inputFile, FILE *outputFile)
{
    if (!inputFile || !outputFile)
        return DATA_FAILURE;

    Header *tempHeader = create_header();

    if (write_header(outputFile, tempHeader) == HEADER_FAILURE)
        return DATA_FAILURE;

    char buffer[BUF_SIZE];
    fgets(buffer, BUF_SIZE, inputFile); // discarded since first line just defines columns

    int numData = 0;
    while (fgets(buffer, sizeof(buffer), inputFile))
    {
        Data *newData = tokenize(buffer);

        newData->removed = '0';
        newData->next = tempHeader->top;

        write_data(outputFile, newData);
        numData++;

        destroy_data(&newData);
    }

    // TODO: numStations, numPairStations
    tempHeader->nextRRN = numData;

    if (write_header(outputFile, tempHeader) == HEADER_FAILURE)
        return DATA_FAILURE;

    return DATA_SUCCESS;
}

Data *read_data(FILE *binFile)
{
    long start = ftell(binFile);

    char removed;
    if (fread(&removed, 1, 1, binFile) != 1)
        return NULL;

    if (fseek(binFile, 4, SEEK_CUR))
        return NULL;

    Data *tmpData = calloc(1, sizeof(Data));

    int intBuf = 0;

    fread(&tmpData->stationCode, sizeof(int), 1, binFile);
    fread(&tmpData->lineCode, sizeof(int), 1, binFile);

    fread(&tmpData->nextStationCode, sizeof(int), 1, binFile);
    fread(&tmpData->distNextStation, sizeof(int), 1, binFile);

    fread(&tmpData->codeIntegLine, sizeof(int), 1, binFile);
    fread(&tmpData->codeIntegStation, sizeof(int), 1, binFile);

    fread(&intBuf, sizeof(int), 1, binFile);
    tmpData->sizeStationName = intBuf;
    if (intBuf > 0)
    {
        tmpData->stationName = malloc(sizeof(char) * (intBuf + 1));
        fread(tmpData->stationName, intBuf, 1, binFile);
        tmpData->stationName[intBuf] = '\0';
    }
    else
        tmpData->stationName = NULL;

    fread(&intBuf, sizeof(int), 1, binFile);
    tmpData->sizeLineName = intBuf;
    if (intBuf > 0)
    {
        tmpData->lineName = malloc(sizeof(char) * (intBuf + 1));
        fread(tmpData->lineName, intBuf, 1, binFile);
        tmpData->lineName[intBuf] = '\0';
    }
    else
        tmpData->stationName = NULL;

    if (fseek(binFile, start + 80, SEEK_SET) != 0)
    {
        free(tmpData);
        return NULL;
    }

    return tmpData;
}

void print_data(Data *data)
{
    if (!data)
        return;

    print_int_or_null(data->stationCode);
    printf(" ");
    print_str_or_null(data->sizeStationName, data->stationName);
    printf(" ");

    print_int_or_null(data->lineCode);
    printf(" ");
    print_str_or_null(data->sizeLineName, data->lineName);
    printf(" ");

    print_int_or_null(data->nextStationCode);
    printf(" ");
    print_int_or_null(data->distNextStation);
    printf(" ");

    print_int_or_null(data->codeIntegLine);
    printf(" ");
    print_int_or_null(data->codeIntegStation);
    printf("\n");
}

int print_all_data(FILE *binFile)
{
    if (!binFile)
        return DATA_FAILURE;

    if (fseek(binFile, HEADER_SIZE, SEEK_SET))
        return DATA_FAILURE;

    Data *tmpData;
    while ((tmpData = read_data(binFile)))
    {
        print_data(tmpData);
        destroy_data(&tmpData);
    }

    return DATA_SUCCESS;
}

void destroy_data(Data **data)
{
    if (!data || !(*data))
        return;

    free((*data)->stationName);
    free((*data)->lineName);
    free(*data);
    *data = NULL;
}