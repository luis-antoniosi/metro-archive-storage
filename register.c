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

Register *parse_register(char *buffer)
{
    Register *tmpRegister = malloc(sizeof(Register));
    if (!tmpRegister)
        return NULL;

    char *ptr = buffer;
    char *token;

    token = custom_strtok(&ptr, ',');
    tmpRegister->stationCode = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    if (token && token[0] != '\0')
    {
        tmpRegister->stationName = strdup(token); // strdup already allocates size
        tmpRegister->sizeStationName = strlen(token);
    }
    else
    {
        tmpRegister->stationName = NULL;
        tmpRegister->sizeStationName = 0;
    }

    token = custom_strtok(&ptr, ',');
    tmpRegister->lineCode = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    if (token && token[0] != '\0')
    {
        tmpRegister->lineName = strdup(token);
        tmpRegister->sizeLineName = strlen(token);
    } 
    else
    {
        tmpRegister->lineName = NULL;
        tmpRegister->sizeLineName = 0;
    }

    token = custom_strtok(&ptr, ',');
    tmpRegister->nextStationCode = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    tmpRegister->distNextStation = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    tmpRegister->codeIntegLine = check_for_null(token);

    token = custom_strtok(&ptr, ',');
    tmpRegister->codeIntegStation = check_for_null(token);

    return tmpRegister;
}

void write_register(FILE *binFile, Register *data)
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

    int remainingBytes = REGISTER_SIZE - newDataSize;
    if (remainingBytes > 0)
    {
        char tmp = TRASH;
        for (int i = 0; i < remainingBytes; i++)
        {
            fwrite(&tmp, sizeof(tmp), 1, binFile);
        }
    }
}

Register *read_register(FILE *binFile)
{
    long start = ftell(binFile);

    char removed;
    if (fread(&removed, 1, 1, binFile) != 1)
        return NULL;

    if (fseek(binFile, 4, SEEK_CUR))
        return NULL;

    Register *tmpRegister = calloc(1, sizeof(Register));
    if (!tmpRegister)
        return NULL;

    tmpRegister->removed = removed;
    // read fixed-size integers
    if (fread(&tmpRegister->stationCode, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpRegister->lineCode, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpRegister->nextStationCode, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpRegister->distNextStation, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpRegister->codeIntegLine, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpRegister->codeIntegStation, sizeof(int), 1, binFile) != 1 ||
        fread(&tmpRegister->sizeStationName, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read general attributes of a register.\n");
        free(tmpRegister);
        return NULL;
    }

    // read station name
    if (tmpRegister->sizeStationName > 0)
    {
        tmpRegister->stationName = malloc(sizeof(char) * (tmpRegister->sizeStationName + 1));
        if (fread(tmpRegister->stationName, tmpRegister->sizeStationName, 1, binFile) != 1)
        {
            printf("Unable to read station name.\n");
            free(tmpRegister->stationName);
            free(tmpRegister);
            return NULL;
        }
        tmpRegister->stationName[tmpRegister->sizeStationName] = '\0';
    }
    else
        tmpRegister->stationName = NULL;

    // read line's name size + line's name
    if (fread(&tmpRegister->sizeLineName, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read line's name size.\n");
        free(tmpRegister->stationName);
        free(tmpRegister);
        return NULL;
    }

    if (tmpRegister->sizeLineName > 0)
    {
        tmpRegister->lineName = malloc(sizeof(char) * (tmpRegister->sizeLineName + 1));
        if (fread(tmpRegister->lineName, tmpRegister->sizeLineName, 1, binFile) != 1)
        {
            printf("Unable to read line name.\n");
            free(tmpRegister->stationName);
            free(tmpRegister->lineName);
            free(tmpRegister);
            return NULL;
        };
        tmpRegister->lineName[tmpRegister->sizeLineName] = '\0';
    }
    else
        tmpRegister->lineName = NULL;

    if (fseek(binFile, start + REGISTER_SIZE, SEEK_SET) != 0)
    {
        free(tmpRegister->stationName);
        free(tmpRegister->lineName);
        free(tmpRegister);
        return NULL;
    }

    return tmpRegister;
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

// Search related

static int check_match(Register *data, SearchField field)
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

Register *check_register_field_search(FILE *binFile, SearchField *filters, int pairIterations, int *isMatch)
{
    Register *tmpRegister = NULL;
    *isMatch = 1;

    if (!(tmpRegister = read_register(binFile)))
        return NULL;

    if (tmpRegister->removed == '1')
    {
        destroy_register(&tmpRegister);
        *isMatch = 0;
        return NULL;
    }

    for (int i = 0; i < pairIterations; i++)
    {
        if (!check_match(tmpRegister, filters[i]))
        {
            *isMatch = 0;
            break;
        }
    }

    return tmpRegister;
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

void remove_register(FILE *binFile)
{
    char removed = '1';

    fseek(binFile, -REGISTER_SIZE, SEEK_CUR);
    fwrite(&removed, sizeof(char), 1, binFile);
}

//

void destroy_register(Register **data)
{
    if (!data || !(*data))
        return;

    free((*data)->stationName);
    free((*data)->lineName);
    free(*data);
    *data = NULL;
}