#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "register.h"

// Helper functions are all static since they aren't used outside of this file

// Parsing, writing and reading

static char *custom_strtok(char **buff, char delim)
{
    if (!buff || !(*buff))
        return NULL;

    char *start = *buff;
    char *delimPos = strchr(start, delim);

    if (delimPos)
    {
        *delimPos = '\0';
        *buff = delimPos + 1;
    }
    else
        *buff = NULL;

    return start;
}

static int check_for_null(char *str)
{
    return !str || str[0] == '\0' || strcspn(str, "\n") == 0 ? -1 : atoi(str);
}

Data *tokenize(char *buffer)
{
    Data *tempData = malloc(sizeof(Data));
    if (!tempData)
        return NULL;

    char *ptr = buffer;
    char *token;

    token = custom_strtok(&ptr, ',');
    tempData->stationCode = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    if (token && token[0] != '\0')
    {
        tempData->stationName = strdup(token); // strdup already allocates size
        tempData->sizeStationName = strlen(token);
    }
    else
    {
        tempData->stationName = NULL;
        tempData->sizeStationName = 0;
    }

    token = custom_strtok(&ptr, ',');
    tempData->lineCode = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    if (token && token[0] != '\0')
    {
        tempData->lineName = strdup(token);
        tempData->sizeLineName = strlen(token);
    } 
    else
    {
        tempData->lineName = NULL;
        tempData->sizeLineName = 0;
    }

    token = custom_strtok(&ptr, ',');
    tempData->nextStationCode = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    tempData->distNextStation = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    tempData->codeIntegLine = check_for_null(token);

    token = custom_strtok(&ptr, ',');
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

Data *read_data(FILE *binFile)
{
    long start = ftell(binFile);

    char removed;
    if (fread(&removed, 1, 1, binFile) != 1)
        return NULL;

    if (fseek(binFile, 4, SEEK_CUR))
        return NULL;

    Data *tmpData = calloc(1, sizeof(Data));
    if (!tmpData)
        return NULL;

    tmpData->removed = removed;
    // read fixed-size integers
    if (fread(&tmpData->stationCode, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpData->lineCode, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpData->nextStationCode, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpData->distNextStation, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpData->codeIntegLine, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpData->codeIntegStation, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpData->sizeStationName, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read general attributes of a register.\n");
        free(tmpData);
        return NULL;
    }

    // read station name
    if (tmpData->sizeStationName > 0)
    {
        tmpData->stationName = malloc(sizeof(char) * (tmpData->sizeStationName + 1));
        if (fread(tmpData->stationName, tmpData->sizeStationName, 1, binFile) != 1)
        {
            printf("Unable to read station name.\n");
            free(tmpData->stationName);
            free(tmpData);
            return NULL;
        }
        tmpData->stationName[tmpData->sizeStationName] = '\0';
    }
    else
        tmpData->stationName = NULL;

    // read line's name size + line's name
    if (fread(&tmpData->sizeLineName, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read line's name size.\n");
        free(tmpData->stationName);
        free(tmpData);
        return NULL;
    }

    if (tmpData->sizeLineName > 0)
    {
        tmpData->lineName = malloc(sizeof(char) * (tmpData->sizeLineName + 1));
        if (fread(tmpData->lineName, tmpData->sizeLineName, 1, binFile) != 1)
        {
            printf("Unable to read line name.\n");
            free(tmpData->stationName);
            free(tmpData->lineName);
            free(tmpData);
            return NULL;
        };
        tmpData->lineName[tmpData->sizeLineName] = '\0';
    }
    else
        tmpData->lineName = NULL;

    if (fseek(binFile, start + DATA_SIZE, SEEK_SET) != 0)
    {
        free(tmpData->stationName);
        free(tmpData->lineName);
        free(tmpData);
        return NULL;
    }

    return tmpData;
}

// Printing related

static void print_int_or_null(int value)
{
    if (value == -1)
        printf("NULO");
    else
        printf("%d", value);
}

static void print_str_or_null(int size, char *str)
{
    if (size == 0 || str == NULL || str[0] == '\0')
        printf("NULO");
    else
        printf("%s", str);
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

// Search related

static int check_match(Data *data, SearchField field)
{
    if (strcmp(field.name, "codEstacao") == 0)
        return data->stationCode == atoi(field.value);
    else if (strcmp(field.name, "codLinha") == 0)
        return data->lineCode == atoi(field.value);
    else if (strcmp(field.name, "codProxEstacao") == 0)
        return data->nextStationCode == atoi(field.value);
    else if (strcmp(field.name, "distProxEstacao") == 0)
        return data->distNextStation == atoi(field.value);
    else if (strcmp(field.name, "codLinhaIntegra") == 0)
        return data->codeIntegLine == atoi(field.value);
    else if (strcmp(field.name, "codEstIntegra") == 0)
        return data->codeIntegStation == atoi(field.value);
    else if (strcmp(field.name, "nomeEstacao") == 0)
        return (strcmp(field.value, data->stationName) == 0);
    else if (strcmp(field.name, "nomeLinha") == 0)
        return (strcmp(field.value, data->lineName) == 0);

    return 0;
}

Data *check_data_field_search(FILE *binFile, SearchField *filters, int pairIterations, int *isMatch)
{
    Data *tmpData = NULL;
    *isMatch = 1;

    if (!(tmpData = read_data(binFile)))
        return NULL;

    if (tmpData->removed == '1')
    {
        destroy_data(&tmpData);
        *isMatch = 0;
        return NULL;
    }

    for (int i = 0; i < pairIterations; i++)
    {
        if (!check_match(tmpData, filters[i]))
        {
            *isMatch = 0;
            break;
        }
    }

    return tmpData;
}

SearchField *get_all_search_fields(int *pairIterations)
{
    char buff[BUF_SIZE];

    if (!fgets(buff, BUF_SIZE, stdin))
        return NULL;

    char *token = strtok(buff, " \n\r");
    if (!token)
        return NULL;

    *pairIterations = atoi(token);

    SearchField *filters = malloc(sizeof(SearchField) * *pairIterations);
    for (int j = 0; j < *pairIterations; j++)
    {
        token = strtok(NULL, " \n\r");
        if (token) // field's name never has quotes
            strcpy(filters[j].name, token);

        token = strtok(NULL, " \n\r");
        if (token && token[0] == '\"') // checking if token (field's value) has quotes, like "Luz" instead of Luz
        {
            char *insideQuotes = strtok(token + 1, "\"");
            if (insideQuotes)
                strcpy(filters[j].value, insideQuotes);
        }
        else if (token)
        {
            strcpy(filters[j].value, token);
        }
    }

    return filters;
}

// Delete

void remove_data(FILE *binFile)
{
    char removed = '1';

    fseek(binFile, -DATA_SIZE, SEEK_CUR);
    fwrite(&removed, sizeof(char), 1, binFile);
}

//

void destroy_data(Data **data)
{
    if (!data || !(*data))
        return;

    free((*data)->stationName);
    free((*data)->lineName);
    free(*data);
    *data = NULL;
}