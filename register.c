#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "register.h"

// Helper functions are all static since they aren't used outside of this file

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

/**
 * @brief Parses a delimited string buffer and populates a register
 *
 * This function takes a line of text and splits it by commas.
 *
 * @param buffer Pointer to a string that represents a single record.
 *
 * @return Register* A pointer to the allocated Register or NULL if the allocation fails
 */
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

/**
 * @brief Writes a Register struct into a binary format
 *
 * @param binFile A pointer to the open binary file
 * @param data A pointer to the struct to be written
 */
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

/**
 * @brief reads a single record from a binary file into a register struct
 *
 * @param binFile A pointer to the open binary file
 *
 * @return Register* Pointer to the dinamically allocated register or NULL if
 *  the end of the file is reached, a read error ocurr or the allocation fails
 */
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

/**
 * @brief Prints a single Register
 *
 * @param data Pointer to the register
 */
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
        return (data->stationName != NULL) && (strcmp(field.value, data->stationName) == 0);
    else if (strcmp(field.name, "nomeLinha") == 0)
        return (data->lineName != NULL) && (strcmp(field.value, data->lineName) == 0);

    return 0;
}

static char *check_quotes(char *str)
{
    if (str)
    {
        if (str[0] == '\"')
        {
            char *insideQuotes = str + 1;
            char *closingQuote = strchr(insideQuotes, '\"');
            if (closingQuote)
                *closingQuote = '\0';

            return insideQuotes;
        }
        else
        {
            return str;
        }
    }

    return NULL;
}

/**
 * @brief reads a Register from a file and evaluates if it meets all the serch filters
 *
 * The function extracts the next register from the binFile and compares it to a array of filters
 * applying the 'AND' logic.
 *
 * @param binFile Pointer to the binary file
 * @param filters array containing the search filters
 * @param pairInterations Number of filters in the array
 * @param isMatch return pointer. 1 if the register pass through all the filters, 0 if not
 *
 * @return Register* Pointer to the read register or NULL at EOF.
 */
Register *check_register_field_search(FILE *binFile, SearchField *filters, int pairIterations)
{
    Register *tmpRegister = NULL;

    while ((tmpRegister = read_register(binFile))) // this loop skips any removed registers
    {
        if (tmpRegister->removed == '1')
        {
            destroy_register(&tmpRegister);
            continue;
        }

        int match = 1;
        for (int i = 0; i < pairIterations; i++)
        {
            if (!check_match(tmpRegister, filters[i]))
            {
                match = 0;
                break;
            }
        }

        if (match)
            return tmpRegister;

        destroy_register(&tmpRegister);
    }

    return NULL;
}

/**
 * @brief Reads the search filters typed by the user
 *
 * @param pairInterations Pointer to the int variable that the number of filters is assigned to
 *
 * @return SearchField* Allocated array containing the filters in a struct or NULL in case of failure,
 *  the caller must free the dinamically alocated array.
 */
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

        token = check_quotes(strtok(NULL, " \n\r"));

        if (token)
            strcpy(filters[j].value, token);

        if (strcmp(filters[j].value, "NULO") == 0) // checking if the value is NULL
            strcpy(filters[j].value, "-1");
    }

    return filters;
}

// Delete

/**
 * @brief removes a register by setting the removed flag and pushing on the RRN stack
 *
 * @param binFile Open binary file
 */
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

DataStatus update_station_counts(FILE *binFile, Header *header)
{
    fseek(binFile, HEADER_SIZE, SEEK_SET);

    char **seenStations = malloc(EXPECTED_SIZE * sizeof(char *));
    Pair *seenPairs = malloc(EXPECTED_SIZE * sizeof(Pair));

    if (!seenStations || !seenPairs || !header)
    {
        free(seenStations);
        free(seenPairs);
        return DATA_FAILURE;
    }

    int numStations = 0, numPairStations = 0;
    Register *tmpRegister = NULL;
    while ((tmpRegister = read_register(binFile)))
    {
        if (tmpRegister->removed == '1')
        {
            destroy_register(&tmpRegister);
            continue;
        }

        if (tmpRegister->stationName)
        {
            int foundName = 0;
            for (int i = 0; i < numStations; i++)
            {
                if (strcmp(seenStations[i], tmpRegister->stationName) == 0)
                {
                    foundName = 1;
                    break;
                }
            }

            if (!foundName)
                seenStations[numStations++] = strdup(tmpRegister->stationName);
        }

        if (tmpRegister->nextStationCode != -1)
        {
            int foundPair = 0;
            // impossibilitating cases like (1, 2) != (2, 1)
            int first = (tmpRegister->stationCode < tmpRegister->nextStationCode) ? tmpRegister->stationCode : tmpRegister->nextStationCode;
            int scnd = (tmpRegister->stationCode < tmpRegister->nextStationCode) ? tmpRegister->nextStationCode : tmpRegister->stationCode;
            for (int i = 0; i < numPairStations; i++)
            {
                if (seenPairs[i].stationCode == first && seenPairs[i].nextStationCode == scnd)
                {
                    foundPair = 1;
                    break;
                }
            }

            if (!foundPair)
            {
                seenPairs[numPairStations].stationCode = first;
                seenPairs[numPairStations].nextStationCode = scnd;
                numPairStations++;
            }
        }

        destroy_register(&tmpRegister);
    }

    header->numStations = numStations;
    header->numPairStations = numPairStations;

    for (int i = 0; i < numStations; i++)
        free(seenStations[i]);
    free(seenStations);
    free(seenPairs);

    return DATA_SUCCESS;
}

//

Register *input_register()
{
    char buff[BUF_SIZE];
    Register *tmpRegister = malloc(sizeof(Register));
    if (!tmpRegister)
        return NULL;

    if (!fgets(buff, BUF_SIZE, stdin))
        return NULL;

    tmpRegister->removed = '0';
    tmpRegister->next = -1;

    char *token = strtok(buff, " \n\r");

    tmpRegister->stationCode = check_for_null(token);

    token = check_quotes(strtok(NULL, " \n\r"));
    if (token && token[0] != '\0')
    {
        tmpRegister->sizeStationName = strlen(token);
        tmpRegister->stationName = strdup(token);
    }
    else
    {
        tmpRegister->stationName = NULL;
        tmpRegister->sizeStationName = 0;
    }

    token = strtok(NULL, " \n\r");
    tmpRegister->lineCode = check_for_null(token);

    token = check_quotes(strtok(NULL, " \n\r"));
    if (token && token[0] != '\0')
    {
        tmpRegister->sizeLineName = strlen(token);
        tmpRegister->lineName = strdup(token);
    }
    else
    {
        tmpRegister->lineName = NULL;
        tmpRegister->sizeLineName = 0;
    }

    token = strtok(NULL, " \n\r");
    tmpRegister->nextStationCode = check_for_null(token);

    token = strtok(NULL, " \n\r");
    tmpRegister->distNextStation = check_for_null(token);

    token = strtok(NULL, " \n\r");
    tmpRegister->codeIntegLine = check_for_null(token);

    token = strtok(NULL, " \n\r");
    tmpRegister->codeIntegStation = check_for_null(token);

    return tmpRegister;
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

/**
 * @brief free the memory of a register
 *
 * @param data double pointer to the register
 */
void destroy_register(Register **data)
{
    if (!data || !(*data))
        return;

    free((*data)->stationName);
    free((*data)->lineName);
    free(*data);
    *data = NULL;
}