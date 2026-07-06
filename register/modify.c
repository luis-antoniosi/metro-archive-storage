#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binFile/dataFile.h" // just for the DataHeader struct type
#include "modify.h"
#include "parseUtils.h" // check_quotes and check_for_null

// Insert, delete, update.

Register *input_register()
{
    char buff[BUF_SIZE];
    Register *currentRegister = malloc(sizeof(Register));
    if (!currentRegister)
        return NULL;

    if (!fgets(buff, BUF_SIZE, stdin))
    {
        free(currentRegister);
        return NULL;
    }

    currentRegister->removed = RECORD_ACTIVE;
    currentRegister->next = -1;

    char *token = strtok(buff, " \n\r");

    currentRegister->stationCode = check_for_null(token);

    char quoteBuf[BUF_SIZE];
    token = check_quotes(strtok(NULL, " \n\r"), quoteBuf);
    if (token && token[0] != '\0')
    {
        currentRegister->sizeStationName = strlen(token);
        currentRegister->stationName = strdup(token);
    }
    else
    {
        currentRegister->stationName = NULL;
        currentRegister->sizeStationName = 0;
    }

    token = strtok(NULL, " \n\r");
    currentRegister->lineCode = check_for_null(token);

    token = check_quotes(strtok(NULL, " \n\r"), quoteBuf);
    if (token && token[0] != '\0')
    {
        currentRegister->sizeLineName = strlen(token);
        currentRegister->lineName = strdup(token);
    }
    else
    {
        currentRegister->lineName = NULL;
        currentRegister->sizeLineName = 0;
    }

    token = strtok(NULL, " \n\r");
    currentRegister->nextStationCode = check_for_null(token);

    token = strtok(NULL, " \n\r");
    currentRegister->distNextStation = check_for_null(token);

    token = strtok(NULL, " \n\r");
    currentRegister->codeIntegLine = check_for_null(token);

    token = strtok(NULL, " \n\r");
    currentRegister->codeIntegStation = check_for_null(token);

    return currentRegister;
}

// Insertion

Status insert_register(FILE *binFile, Register *data, DataHeader *header)
{
    if (!binFile || !data || !header)
        return FAILURE;

    int insertOffset = 0;

    if (header->top != -1)
    {
        insertOffset = header->top * REGISTER_SIZE;
        int nextTop = 0;

        if (fseek(binFile, insertOffset + HEADER_SIZE + sizeof(char), SEEK_SET)) // skipping "removed"
            return FAILURE;

        if (fread(&nextTop, sizeof(int), 1, binFile) != 1)
            return FAILURE;

        header->top = nextTop;
    }
    else
    {
        insertOffset = header->nextRRN * REGISTER_SIZE;
        header->nextRRN++;
    }

    if (fseek(binFile, insertOffset + HEADER_SIZE, SEEK_SET))
        return FAILURE;

    if (write_register(binFile, data) == FAILURE)
        return FAILURE;

    return SUCCESS;
}

// Delete

Status remove_register(FILE *binFile, int removedRRN)
{
    char removed = '1';

    int registerStart = HEADER_SIZE + (REGISTER_SIZE * removedRRN);
    // seek to and write removed flag
    if (fseek(binFile, registerStart, SEEK_SET) ||
        fwrite(&removed, sizeof(char), 1, binFile) != 1)
        return FAILURE;

    // seek top position and read value
    if (fseek(binFile, sizeof(char), SEEK_SET))
        return FAILURE;

    int topValue = 0;
    if (fread(&topValue, sizeof(int), 1, binFile) != 1)
        return FAILURE;

    // write removed register byte offset in the header top field
    if (fseek(binFile, sizeof(char), SEEK_SET) ||
        fwrite(&removedRRN, sizeof(int), 1, binFile) != 1)
        return FAILURE;

    // update the "next" field of the register to old top value
    if (fseek(binFile, registerStart + sizeof(char), SEEK_SET) ||
        fwrite(&topValue, sizeof(int), 1, binFile) != 1)
        return FAILURE;

    return SUCCESS;
}

// Update

// essentially the same as check_match from search.c, but applies the field instead
/**
 * @brief Updates a field of a Register to a SearchField's value
 *
 * @param data Pointer to the register to be updated
 * @param field Populated SearchField struct with the field's name and the new value to be assigned
 */
static Status update_match(Register *data, SearchField field)
{
    if (strcmp(field.name, "codEstacao") == 0)
    {
        data->stationCode = atoi(field.value);
        return SUCCESS;
    }
    else if (strcmp(field.name, "codLinha") == 0)
    {
        data->lineCode = atoi(field.value);
        return SUCCESS;
    }
    else if (strcmp(field.name, "codProxEstacao") == 0)
    {
        data->nextStationCode = atoi(field.value);
        return SUCCESS;
    }
    else if (strcmp(field.name, "distProxEstacao") == 0)
    {
        data->distNextStation = atoi(field.value);
        return SUCCESS;
    }
    else if (strcmp(field.name, "codLinhaIntegra") == 0)
    {
        data->codeIntegLine = atoi(field.value);
        return SUCCESS;
    }
    else if (strcmp(field.name, "codEstIntegra") == 0)
    {
        data->codeIntegStation = atoi(field.value);
        return SUCCESS;
    }
    else if (strcmp(field.name, "nomeEstacao") == 0)
    {
        if (data->stationName)
            free(data->stationName);

        data->stationName = strdup(field.value);
        if (!data->stationName)
            return FAILURE;
        data->sizeStationName = strlen(data->stationName);

        return SUCCESS;
    }
    else if (strcmp(field.name, "nomeLinha") == 0)
    {
        if (data->lineName)
            free(data->lineName);

        data->lineName = strdup(field.value);
        if (!data->lineName)
            return FAILURE;
        data->sizeLineName = strlen(data->lineName);

        return SUCCESS;
    }

    return FAILURE;
}

Status update_register(FILE *binFile, Register *data, SearchField *filters, int iterations)
{
    if (!data)
        return FAILURE;

    Status updateStatus = SUCCESS;
    for (int i = 0; i < iterations; i++)
    {
        if (update_match(data, filters[i]) == FAILURE)
        {
            updateStatus = FAILURE;
            break;
        }
    }

    if (updateStatus == SUCCESS)
    {
        // writes updated register
        if (fseek(binFile, -REGISTER_SIZE, SEEK_CUR))
            return FAILURE;

        if (write_register(binFile, data) == FAILURE)
            return FAILURE;
    }

    return SUCCESS;
}