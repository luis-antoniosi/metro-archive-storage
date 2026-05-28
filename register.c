#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "register.h"

// Helper functions are all static since they aren't used outside of this file
// Only the helper functions are commented because they are not declared in the .h file

// Parsing, writing and reading

/**
 * @brief Optimized strtok to deal with consecutive delimiter characters
 * @param buff Double pointer to the string to be parsed. The inside pointer is
 *  updated in each call to point to the beggining of the next token
 * @param delim Delimiter char
 * @return Pointer to the beggining of the token, or NULL if the string ended
 *  or buff is invalid.
 */
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

/**
 * @brief evaluates a string and converts it to an integer
 *
 * This function verifies if the string is NULL, empty or is a single '\n'
 * character. If is valid, it parses it into a integer.
 *
 * @param str Pointer to the string
 *
 * @return Converted integer or -1 if the string is NULL, empty or '\n'
 */
static int check_for_null(char *str)
{
    return !str || str[0] == '\0' || strcspn(str, "\r\n") == 0 || strcmp(str, "NULO") == 0 ? -1 : atoi(str);
}

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

void write_register(FILE *binFile, Register *data)
{
    //variable to count bytes written, instead of ftell
    int bytesWritten = 0;

    fwrite(&data->removed, sizeof(char), 1, binFile);
    bytesWritten += sizeof(char);

    fwrite(&data->next, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);

    //write numeric fields with fixed size
    fwrite(&data->stationCode, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);
    
    fwrite(&data->lineCode, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);

    fwrite(&data->nextStationCode, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);
    
    fwrite(&data->distNextStation, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);

    fwrite(&data->codeIntegLine, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);
    
    fwrite(&data->codeIntegStation, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);

    // write station name size and the station name itself
    fwrite(&data->sizeStationName, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);
    if (data->sizeStationName > 0)
    {
        fwrite(data->stationName, data->sizeStationName, 1, binFile);
        bytesWritten += data->sizeStationName;
    }

    // writes line name size and the line name itself
    fwrite(&data->sizeLineName, sizeof(int), 1, binFile);
    bytesWritten += sizeof(int);
    if (data->sizeLineName > 0)
    {
        fwrite(data->lineName, data->sizeLineName, 1, binFile);
        bytesWritten += data->sizeLineName;
    }

    //calculates remaining space and fills it with garbage ($)
    int remainingBytes = REGISTER_SIZE - bytesWritten;
    if (remainingBytes > 0)
    {
        char trash = TRASH;
        for (int i = 0; i < remainingBytes; i++)
        {
            fwrite(&trash, sizeof(trash), 1, binFile);
        }
    }
}

Register *read_register(FILE *binFile)
{
    //variable to count how many bytes read, instead of ftell
    int bytesRead = 0;

    
    Register *currentRegister = calloc(1, sizeof(Register));
    if (!currentRegister)
        return NULL;

    // these two freads are separate because the -Og flag in gcc affects the order
    //reads just the removed status
    if (fread(&currentRegister->removed, sizeof(char), 1, binFile) != 1)
    {
        free(currentRegister);
        return NULL;
    }
    bytesRead += sizeof(char);

    //correction: if register is flaged as removed, skip to the next register
    if(currentRegister->removed == '1'){
        fseek(binFile, REGISTER_SIZE - bytesRead, SEEK_CUR);
        return currentRegister;
    }

    //if not removed, go on reading normally
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

    //read line's name
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

    //instead of using ftell or fseek with SEEK_SET, we calculate how many bytes are remaining and 
    //use relative fseek with SEEK_CUR foward
    int remainingBytes = REGISTER_SIZE - bytesRead;
    if (remainingBytes > 0)
    {
        fseek(binFile, remainingBytes, SEEK_CUR);
    }

    return currentRegister;
}

// Printing related

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

// Search related

/**
 * @brief Compare a Register field with a SearchFiel value
 *
 * @param data Pointer to the register with the file data
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

/**
 * @brief remakes a string of more than one word that was parsed by strtok
 *
 * @param str initial string token
 * @param buff already allocated array of char used to controy the string
 *
 * @return char* Pointer to the reconstructed string or the original string pointer
 */
static char *check_quotes(char *str, char *buf)
{
    if (!str)
        return NULL;

    if (str[0] == '\"')
    {
        buf[0] = '\0';
        char *insideQuotes = str + 1;

        while (insideQuotes)
        {
            char *closingQuotes = strchr(insideQuotes, '\"');
            if (closingQuotes)
            {
                *closingQuotes = '\0';
                strcat(buf, insideQuotes);
                return buf;
            }

            strcat(buf, insideQuotes);
            strcat(buf, " ");
            insideQuotes = strtok(NULL, " \n\r");
            if (!insideQuotes)
                return buf;
        }
    }

    return str;
}

Register *check_register_field_search(FILE *binFile, SearchField *filters, int pairIterations)
{
    Register *currentRegister = NULL;

    while ((currentRegister = read_register(binFile))) // this loop skips any removed registers
    {
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
        token = check_quotes(strtok(NULL, " \n\r"), quoteBuf);

        if (token)
            strcpy(filters[j].value, token);

        if (strcmp(filters[j].value, "NULO") == 0) // checking if the value is NULL
            strcpy(filters[j].value, "-1");
    }

    return filters;
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

Register *input_register()
{
    char buff[BUF_SIZE];
    Register *currentRegister = malloc(sizeof(Register));
    if (!currentRegister)
        return NULL;

    if (!fgets(buff, BUF_SIZE, stdin))
        return NULL;

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

DataStatus insert_register(FILE *binFile, Register *data, Header *header)
{
    if (!binFile || !data || !header)
        return DATA_FAILURE;

    int nextPos = 0, nextPosReplacement = 0;

    if (header->top != -1)
    {
        nextPos = header->top * REGISTER_SIZE;
        fseek(binFile, nextPos + HEADER_SIZE + sizeof(char), SEEK_SET); // skipping "removed"
        if (fread(&nextPosReplacement, sizeof(int), 1, binFile) != 1)
            return DATA_FAILURE;

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

    return DATA_SUCCESS;
}

//

// essentially the same as check_match, but applies the field instead
/**
 * @brief Updates a field of a Register
 *
 * @param data Pointer to the register to be updated
 * @param field populated SearchField struct with the field`s name and the new value to be assigned
 */
static DataStatus update_match(Register *data, SearchField field)
{
    if (strcmp(field.name, "codEstacao") == 0)
    {
        data->stationCode = atoi(field.value);
        return DATA_SUCCESS;
    }
    else if (strcmp(field.name, "codLinha") == 0)
    {
        data->lineCode = atoi(field.value);
        return DATA_SUCCESS;
    }
    else if (strcmp(field.name, "codProxEstacao") == 0)
    {
        data->nextStationCode = atoi(field.value);
        return DATA_SUCCESS;
    }
    else if (strcmp(field.name, "distProxEstacao") == 0)
    {
        data->distNextStation = atoi(field.value);
        return DATA_SUCCESS;
    }
    else if (strcmp(field.name, "codLinhaIntegra") == 0)
    {
        data->codeIntegLine = atoi(field.value);
        return DATA_SUCCESS;
    }
    else if (strcmp(field.name, "codEstIntegra") == 0)
    {
        data->codeIntegStation = atoi(field.value);
        return DATA_SUCCESS;
    }
    else if (strcmp(field.name, "nomeEstacao") == 0)
    {
        if (data->stationName)
            free(data->stationName);

        data->stationName = strdup(field.value);
        if (!data->stationName)
            return DATA_FAILURE;
        data->sizeStationName = strlen(data->stationName);

        return DATA_SUCCESS;
    }
    else if (strcmp(field.name, "nomeLinha") == 0)
    {
        if (data->lineName)
            free(data->lineName);

        data->lineName = strdup(field.value);
        if (!data->lineName)
            return DATA_FAILURE;
        data->sizeLineName = strlen(data->lineName);

        return DATA_SUCCESS;
    }

    return DATA_FAILURE;
}

DataStatus update_register(FILE *binFile, Register *data, SearchField *filters, int iterations)
{
    if (!data)
        return DATA_FAILURE;

    int swapWorked = 1;
    for (int i = 0; i < iterations; i++)
    {
        if (update_match(data, filters[i]) == DATA_FAILURE)
        {
            swapWorked = 0;
            break;
        }
    }

    if (swapWorked)
    {
        fseek(binFile, -REGISTER_SIZE, SEEK_CUR);
        write_register(binFile, data);
    }

    return DATA_SUCCESS;
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