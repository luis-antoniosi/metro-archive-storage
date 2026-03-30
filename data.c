#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "data.h"

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

    char buffer[256];
    fgets(buffer, 256, inputFile); // discarded since first line just defines columns

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

void destroy_data(Data **data)
{
    if (!data || !(*data))
        return;

    free((*data)->stationName);
    free((*data)->lineName);
    free(*data);
    *data = NULL;
}