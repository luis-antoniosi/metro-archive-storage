#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "register.h"
#include "parseUtils.h" // custom_strtok and check_for_null

// Parsing, writing, reading and printing.

Register *parse_register(char *buffer)
{
    Register *currentRegister = malloc(sizeof(Register));
    if (!currentRegister)
        return NULL;

    char *ptr = buffer;
    char *token;

    token = custom_strtok(&ptr, ',');
    currentRegister->stationCode = check_for_null(token);

    // If the station code is invalid (empty line or missing primary key),
    // treat this as no record and return NULL so the caller can skip it.
    if (currentRegister->stationCode == -1)
    {
        free(currentRegister);
        return NULL;
    }

    token = custom_strtok(&ptr, ',');
    if (token && token[0] != '\0')
    {
        currentRegister->stationName = strdup(token); // strdup already allocates size
        currentRegister->sizeStationName = strlen(token);
    }
    else
    {
        currentRegister->stationName = NULL;
        currentRegister->sizeStationName = 0;
    }

    token = custom_strtok(&ptr, ',');
    currentRegister->lineCode = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    if (token && token[0] != '\0')
    {
        currentRegister->lineName = strdup(token);
        currentRegister->sizeLineName = strlen(token);
    }
    else
    {
        currentRegister->lineName = NULL;
        currentRegister->sizeLineName = 0;
    }

    token = custom_strtok(&ptr, ',');
    currentRegister->nextStationCode = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    currentRegister->distNextStation = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    currentRegister->codeIntegLine = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    currentRegister->codeIntegStation = check_for_null(token);

    return currentRegister;
}

Status write_register(FILE *binFile, Register *data)
{
    // variable to count bytes written, instead of ftell
    int bytesWritten = 0;

    if (fwrite(&data->removed, sizeof(char), 1, binFile) != 1 ||
        fwrite(&data->next, sizeof(int), 1, binFile) != 1 ||
        fwrite(&data->stationCode, sizeof(int), 1, binFile) != 1 ||
        fwrite(&data->lineCode, sizeof(int), 1, binFile) != 1 ||
        fwrite(&data->nextStationCode, sizeof(int), 1, binFile) != 1 ||
        fwrite(&data->distNextStation, sizeof(int), 1, binFile) != 1 ||
        fwrite(&data->codeIntegLine, sizeof(int), 1, binFile) != 1 ||
        fwrite(&data->codeIntegStation, sizeof(int), 1, binFile) != 1)
        return FAILURE;

    bytesWritten = sizeof(char) + 7 * sizeof(int);

    if (fwrite(&data->sizeStationName, sizeof(int), 1, binFile) != 1)
        return FAILURE;
    bytesWritten += sizeof(int);

    if (data->sizeStationName > 0)
    {
        if (fwrite(data->stationName, data->sizeStationName, 1, binFile) != 1)
            return FAILURE;

        bytesWritten += data->sizeStationName;
    }

    // writes line name size and the line name itself
    if (fwrite(&data->sizeLineName, sizeof(int), 1, binFile) != 1)
        return FAILURE;
    bytesWritten += sizeof(int);

    if (data->sizeLineName > 0)
    {
        if (fwrite(data->lineName, data->sizeLineName, 1, binFile) != 1)
            return FAILURE;

        bytesWritten += data->sizeLineName;
    }

    // calculates remaining space and fills it with garbage ($)
    int remainingBytes = REGISTER_SIZE - bytesWritten;
    if (remainingBytes > 0)
    {
        char trash = TRASH;
        for (int i = 0; i < remainingBytes; i++)
        {
            if (fwrite(&trash, sizeof(trash), 1, binFile) != 1)
                return FAILURE;
        }
    }

    return SUCCESS;
}

Register *read_register(FILE *binFile, int readRemoved)
{
    // variable to count how many bytes read, instead of ftell
    int bytesRead = 0;

    Register *currentRegister = calloc(1, sizeof(Register));
    if (!currentRegister)
        return NULL;

    // reads just the removed status
    if (fread(&currentRegister->removed, sizeof(char), 1, binFile) != 1)
    {
        free(currentRegister);
        return NULL;
    }

    bytesRead += sizeof(char);

    // return the register as is. if it is removed, seek to the next one. all functionalities check if it is removed beforehand.
    if (currentRegister->removed == RECORD_REMOVED && !readRemoved)
    {
        if (fseek(binFile, REGISTER_SIZE - bytesRead, SEEK_CUR))
        {
            free(currentRegister);
            return NULL;
        }
        return currentRegister;
    }

    // if not removed, go on reading normally
    if (fread(&currentRegister->next, sizeof(int), 1, binFile) != 1)
    {
        free(currentRegister);
        return NULL;
    }
    bytesRead += sizeof(int);

    // read fixed-size integers
    if (fread(&currentRegister->stationCode, sizeof(int), 1, binFile) != 1 ||
        fread(&currentRegister->lineCode, sizeof(int), 1, binFile) != 1 ||
        fread(&currentRegister->nextStationCode, sizeof(int), 1, binFile) != 1 ||
        fread(&currentRegister->distNextStation, sizeof(int), 1, binFile) != 1 ||
        fread(&currentRegister->codeIntegLine, sizeof(int), 1, binFile) != 1 ||
        fread(&currentRegister->codeIntegStation, sizeof(int), 1, binFile) != 1 ||
        fread(&currentRegister->sizeStationName, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read general attributes of a register.\n");
        free(currentRegister);
        return NULL;
    }
    bytesRead += 7 * sizeof(int);

    // read station's name
    if (currentRegister->sizeStationName > 0)
    {
        currentRegister->stationName = malloc(sizeof(char) * (currentRegister->sizeStationName + 1));

        if (currentRegister->stationName == NULL || (fread(currentRegister->stationName, currentRegister->sizeStationName, 1, binFile) != 1))
        {
            printf("Unable to read station name.\n");
            free(currentRegister->stationName);
            free(currentRegister);
            return NULL;
        }
        currentRegister->stationName[currentRegister->sizeStationName] = '\0';
        bytesRead += currentRegister->sizeStationName;
    }
    else
        currentRegister->stationName = NULL;

    // read line's name size
    if (fread(&currentRegister->sizeLineName, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read line's name size.\n");
        free(currentRegister->stationName);
        free(currentRegister);
        return NULL;
    }
    bytesRead += sizeof(int);

    // read line's name
    if (currentRegister->sizeLineName > 0)
    {
        currentRegister->lineName = malloc(sizeof(char) * (currentRegister->sizeLineName + 1));
        if (!currentRegister->lineName || fread(currentRegister->lineName, currentRegister->sizeLineName, 1, binFile) != 1)
        {
            printf("Unable to read line name.\n");
            free(currentRegister->stationName);
            free(currentRegister->lineName);
            free(currentRegister);
            return NULL;
        };
        currentRegister->lineName[currentRegister->sizeLineName] = '\0';
        bytesRead += currentRegister->sizeLineName;
    }
    else
        currentRegister->lineName = NULL;

    // instead of using ftell or fseek with SEEK_SET, we calculate how many bytes are remaining and
    // use relative fseek with SEEK_CUR foward
    int remainingBytes = REGISTER_SIZE - bytesRead;
    if (remainingBytes > 0)
    {
        if (fseek(binFile, remainingBytes, SEEK_CUR))
        {
            free(currentRegister->stationName);
            free(currentRegister->lineName);
            free(currentRegister);
            return NULL;
        }
    }

    return currentRegister;
}

/**
 * @brief prints the value or "NULO" if value == -1
 *
 * @param value value to be printed
 */
static void print_int_or_null(int value)
{
    if (value == -1)
        printf("NULO");
    else
        printf("%d", value);
}

/**
 * @brief prints the string or "NULO" if the string is empty or NULL
 *
 * @param size Size of the string
 * @param str pointer to the string
 */
static void print_str_or_null(int size, char *str)
{
    if (size == 0 || str == NULL || str[0] == '\0')
        printf("NULO");
    else
        printf("%s", str);
}

void print_register(Register *data)
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

void destroy_register(Register **data)
{
    if (!data || !(*data))
        return;

    free((*data)->stationName);
    free((*data)->lineName);
    free(*data);
    *data = NULL;
}