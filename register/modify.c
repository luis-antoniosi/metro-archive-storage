#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modify.h"
#include "utils.h"

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

    currentRegister->removed = '0';
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

// change some variable names
Status insert_register(FILE *binFile, Register *data, Header *header)
{
    if (!binFile || !data || !header)
        return FAILURE;

    int nextPos = 0, nextPosReplacement = 0;

    if (header->top != -1)
    {
        nextPos = header->top * REGISTER_SIZE;
        fseek(binFile, nextPos + HEADER_SIZE + sizeof(char), SEEK_SET); // skipping "removed"
        if (fread(&nextPosReplacement, sizeof(int), 1, binFile) != 1)
            return FAILURE;

        header->top = nextPosReplacement;
    }
    else
    {
        nextPos = header->nextRRN * REGISTER_SIZE;
        nextPosReplacement = header->nextRRN + 1;

        header->nextRRN = nextPosReplacement;
    }

    fseek(binFile, nextPos + HEADER_SIZE, SEEK_SET);

    write_register(binFile, data);

    return SUCCESS;
}

// Delete

void remove_register(FILE *binFile)
{
    char removed = '1';

    // rewind to start of register
    fseek(binFile, -REGISTER_SIZE, SEEK_CUR);
    int registerStart = ftell(binFile);

    int removedRRN = (registerStart - HEADER_SIZE) / REGISTER_SIZE;

    // writes the removed flag
    fwrite(&removed, sizeof(char), 1, binFile);

    // seek top position and read value
    fseek(binFile, sizeof(char), SEEK_SET);
    int topValue = 0;
    if (fread(&topValue, sizeof(int), 1, binFile) != 1)
        return;

    // write removed register byte offset in the header top field
    fseek(binFile, sizeof(char), SEEK_SET);
    fwrite(&removedRRN, sizeof(int), 1, binFile);

    // update the "next" field of the register to old top value
    fseek(binFile, registerStart + 1, SEEK_SET);
    fwrite(&topValue, sizeof(int), 1, binFile);

    return;
}

// Update

// essentially the same as check_match, but applies the field instead
/**
 * @brief Updates a field of a Register
 *
 * @param data Pointer to the register to be updated
 * @param field populated SearchField struct with the field's name and the new value to be assigned
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
        fseek(binFile, -REGISTER_SIZE, SEEK_CUR);
        write_register(binFile, data);
    }

    return SUCCESS;
}