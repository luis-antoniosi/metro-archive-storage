#include <stdlib.h>
#include <string.h>
#include "search.h"
#include "parseUtils.h"

// Search, filter.

/**
 * @brief Compares each Register field with a SearchField value
 *
 * @param data Pointer to a Register
 * @param field SearchField containing the column name and value searched
 *
 * @return int Return 1 if equal and 0 if not equal
 */
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
        return data->stationName && strcmp(field.value, data->stationName) == 0;
    else if (strcmp(field.name, "nomeLinha") == 0)
        return data->lineName && strcmp(field.value, data->lineName) == 0;

    return 0;
}

Register *check_register_field_search(FILE *binFile, SearchField *filters, int pairIterations)
{
    Register *currentRegister = NULL;

    while ((currentRegister = read_register(binFile))) 
    {
        // this loop skips any removed registers
        if (currentRegister->removed == '1')
        {
            destroy_register(&currentRegister);
            continue;
        }

        int match = 1;
        for (int i = 0; i < pairIterations; i++)
        {
            if (!check_match(currentRegister, filters[i]))
            {
                match = 0;
                break;
            }
        }

        if (match)
            return currentRegister;

        destroy_register(&currentRegister);
    }

    return NULL;
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

    // need to use calloc so it doesn't have trash in it.
    SearchField *filters = calloc(*pairIterations, sizeof(SearchField));
    for (int j = 0; j < *pairIterations; j++)
    {
        token = strtok(NULL, " \n\r");
        if (token) // field's name never has quotes
            strcpy(filters[j].name, token);

        char quoteBuf[BUF_SIZE];
        token = check_quotes(strtok(NULL, " \n\r"), quoteBuf); // field's value can have quotes

        if (token)
            strcpy(filters[j].value, token);

        if (strcmp(filters[j].value, "NULO") == 0) 
            strcpy(filters[j].value, "-1");
    }

    return filters;
}