#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dataFile.h"

#include "register/register.h"
#include "register/search.h"
#include "register/modify.h"

#include "bTree/bTree.h"

// DataHeader functions

DataHeader *create_data_header()
{
    DataHeader *header = malloc(sizeof(DataHeader));

    if (!header)
        return NULL;

    header->status = STATUS_INCONSISTENT;
    header->top = -1;
    header->nextRRN = 0;
    header->numStations = 0;
    header->numPairStations = 0;

    return header;
}

Status write_data_header(FILE *dataFile, DataHeader *header)
{
    if (!dataFile || !header)
        return FAILURE;

    if (fseek(dataFile, 0, SEEK_SET))
        return FAILURE;

    fwrite(&header->status, sizeof(char), 1, dataFile);
    fwrite(&header->top, sizeof(int), 1, dataFile);
    fwrite(&header->nextRRN, sizeof(int), 1, dataFile);
    fwrite(&header->numStations, sizeof(int), 1, dataFile);
    fwrite(&header->numPairStations, sizeof(int), 1, dataFile);

    return SUCCESS;
}

DataHeader *read_data_header(FILE *dataFile)
{
    if (!dataFile)
        return NULL;

    if (fseek(dataFile, 0, SEEK_SET))
        return NULL;

    DataHeader *header = create_data_header();

    if (fread(&header->status, sizeof(char), 1, dataFile) != 1 ||
        fread(&header->top, sizeof(int), 1, dataFile) != 1 ||
        fread(&header->nextRRN, sizeof(int), 1, dataFile) != 1 ||
        fread(&header->numStations, sizeof(int), 1, dataFile) != 1 ||
        fread(&header->numPairStations, sizeof(int), 1, dataFile) != 1)
    {
        printf("Unable to read header.\n");
        free(header);
        return NULL;
    }

    return header;
}

Status update_data_header_count(FILE *dataFile)
{
    DataHeader *header = read_data_header(dataFile);
    if (!header)
        return FAILURE;

    fseek(dataFile, HEADER_SIZE, SEEK_SET);

    char **seenStations = malloc(EXPECTED_SIZE * sizeof(char *));
    StationPair *seenPairs = malloc(EXPECTED_SIZE * sizeof(StationPair));

    if (!seenStations || !seenPairs || !header)
    {
        free(seenStations);
        free(seenPairs);
        return FAILURE;
    }

    int numStations = 0, numPairStations = 0;
    Register *currentRegister = NULL;

    while ((currentRegister = read_register(dataFile)))
    {
        if (currentRegister->removed == '1')
        {
            destroy_register(&currentRegister);
            continue;
        }

        if (currentRegister->stationName)
        {
            int isDuplicate = 0;
            for (int i = 0; i < numStations; i++)
            {
                // current register's station is already in the array, break
                if (strcmp(seenStations[i], currentRegister->stationName) == 0)
                {
                    isDuplicate = 1;
                    break;
                }
            }

            // current register's station is not in the array, add it
            if (!isDuplicate)
                seenStations[numStations++] = strdup(currentRegister->stationName);
        }

        // processing unique pairs
        if (currentRegister->nextStationCode != -1)
        {
            int isDuplicatePair = 0;
            // impossibilitating cases like (1, 2) != (2, 1)
            int first = (currentRegister->stationCode < currentRegister->nextStationCode) ? currentRegister->stationCode : currentRegister->nextStationCode;
            int second = (currentRegister->stationCode < currentRegister->nextStationCode) ? currentRegister->nextStationCode : currentRegister->stationCode;
            for (int i = 0; i < numPairStations; i++)
            {
                if (seenPairs[i].stationCode == first && seenPairs[i].nextStationCode == second)
                {
                    isDuplicatePair = 1;
                    break;
                }
            }

            if (!isDuplicatePair)
            {
                seenPairs[numPairStations].stationCode = first;
                seenPairs[numPairStations].nextStationCode = second;
                numPairStations++;
            }
        }

        destroy_register(&currentRegister);
    }

    header->numStations = numStations;
    header->numPairStations = numPairStations;

    for (int i = 0; i < numStations; i++)
        free(seenStations[i]);
    free(seenStations);
    free(seenPairs);

    if (write_data_header(dataFile, header) == FAILURE)
        return FAILURE;

    free(header);

    return SUCCESS;
}

void change_status(FILE *dataFile, char status)
{
    fseek(dataFile, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, dataFile);
    fflush(dataFile);
}

// File Functions

Status write_bin_file(FILE *inputFile, FILE *outputFile)
{
    if (!inputFile || !outputFile)
        return FAILURE;

    change_status(outputFile, STATUS_INCONSISTENT);

    DataHeader *fileHeader = create_data_header();
    char buffer[BUF_SIZE]; // BUF_SIZE from types.h
    int numData = 0;

    // essentially this if does three different things, but i didnt want to write 3 different ifs
    if (!fileHeader || write_data_header(outputFile, fileHeader) || !fgets(buffer, BUF_SIZE, inputFile))
    {
        free(fileHeader);
        return FAILURE;
    }

    while (fgets(buffer, sizeof(buffer), inputFile))
    {
        Register *newRegister = parse_register(buffer);
        if (!newRegister)
            continue;

        newRegister->removed = '0';
        newRegister->next = -1;

        write_register(outputFile, newRegister);
        numData++;

        destroy_register(&newRegister);
    }

    fileHeader->nextRRN = numData;

    write_data_header(outputFile, fileHeader);
    free(fileHeader);

    // could pass the fileHeader to it, but I prefered to make the function read the header by itself
    if (update_data_header_count(outputFile) == FAILURE)
        return FAILURE;

    change_status(outputFile, STATUS_CONSISTENT);

    return SUCCESS;
}

// printing related

Status print_all_data(FILE *dataFile)
{
    if (!dataFile)
        return FAILURE;

    if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
        return FAILURE;

    Register *currentReg;
    int foundRegister = 0;
    while ((currentReg = read_register(dataFile)))
    {
        if (currentReg->removed != '1')
        {
            print_register(currentReg);
            foundRegister = 1;
        }

        destroy_register(&currentReg);
    }

    if (!foundRegister)
        printf("Registro inexistente.");

    return SUCCESS;
}

Status print_all_data_where(FILE *dataFile, int iterations)
{
    if (!dataFile)
        return FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
            return FAILURE;

        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        // verifies if the search uses primary key
        int searchByStationCode = 0;
        for (int j = 0; j < pairIterations; j++)
        {
            if (strcmp(filters[j].name, "codEstacao") == 0)
            {
                searchByStationCode = 1;
                break;
            }
        }

        Register *currentReg = NULL;
        int anyMatches = 0; // variable to check if a register was found based on the fields

        while ((currentReg = check_register_field_search(dataFile, filters, pairIterations)))
        {
            print_register(currentReg);

            anyMatches = 1;

            destroy_register(&currentReg);

            // if primary key is used, break
            if (searchByStationCode)
            {
                break;
            }
        }

        if (!anyMatches)
            printf("Registro inexistente.\n");

        printf("\n");

        free(filters);
    }

    return SUCCESS;
}

// delete
// todo: combine this with select?
Status delete_all_data_where(FILE *dataFile, int iterations)
{
    if (!dataFile)
        return FAILURE;

    change_status(dataFile, STATUS_INCONSISTENT);

    for (int i = 0; i < iterations; i++)
    {
        if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
            return FAILURE;

        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        // verifies if the search uses primary key
        int searchByStationCode = 0;
        for (int j = 0; j < pairIterations; j++)
        {
            if (strcmp(filters[j].name, "codEstacao") == 0)
            {
                searchByStationCode = 1;
                break;
            }
        }

        Register *currentReg = NULL;
        while ((currentReg = check_register_field_search(dataFile, filters, pairIterations)))
        {
            remove_register(dataFile);
            fseek(dataFile, REGISTER_SIZE - sizeof(char) - sizeof(int), SEEK_CUR);

            destroy_register(&currentReg);

            // if primary key is used, break
            if (searchByStationCode)
            {
                break;
            }
        }

        free(filters);
    }

    if (update_data_header_count(dataFile) == FAILURE)
        return FAILURE;

    change_status(dataFile, STATUS_CONSISTENT);

    return SUCCESS;
}

Status insert_data(FILE *dataFile, int iterations)
{
    if (!dataFile)
        return FAILURE;

    change_status(dataFile, STATUS_INCONSISTENT);
    DataHeader *currHeader = read_data_header(dataFile);
    if (!currHeader)
        return FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        Register *currentReg = input_register();

        if (currentReg)
        {
            insert_register(dataFile, currentReg, currHeader);
            destroy_register(&currentReg);
        }
    }

    write_data_header(dataFile, currHeader);
    free(currHeader);

    if (update_data_header_count(dataFile) == FAILURE)
        return FAILURE;

    change_status(dataFile, STATUS_CONSISTENT);

    return SUCCESS;
}

// TODO: Combine this with select too?
Status update_data_where(FILE *dataFile, int iterations)
{
    if (!dataFile)
        return FAILURE;

    change_status(dataFile, STATUS_INCONSISTENT);

    for (int i = 0; i < iterations; i++)
    {
        if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
            return FAILURE;

        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        // verifies if the search uses primary key
        int searchByStationCode = 0;
        for (int j = 0; j < pairIterations; j++)
        {
            if (strcmp(filters[j].name, "codEstacao") == 0)
            {
                searchByStationCode = 1;
                break;
            }
        }

        // not really a search field in this case, but can be repurposed.
        int updatePairIterations = 0;
        SearchField *updateFilters = get_all_search_fields(&updatePairIterations);

        Register *currentReg = NULL;
        while ((currentReg = check_register_field_search(dataFile, filters, pairIterations)))
        {
            update_register(dataFile, currentReg, updateFilters, updatePairIterations);

            destroy_register(&currentReg);

            // if primary key is used, break
            if (searchByStationCode)
            {
                break;
            }
        }

        free(filters);
        free(updateFilters);
    }

    change_status(dataFile, STATUS_CONSISTENT);

    return SUCCESS;
}

// didn't really make our own, just copied and changed the variables' names
void binary_on_screen(char *fileName)
{
    FILE *dataFile = NULL;

    if (!fileName || !(dataFile = fopen(fileName, "rb")))
        return;

    fseek(dataFile, 0, SEEK_END);
    long totalBytes = ftell(dataFile);

    fseek(dataFile, 0, SEEK_SET);
    unsigned char *bytesStr = malloc(sizeof(unsigned char) * totalBytes);
    if (fread(bytesStr, 1, totalBytes, dataFile) != (long unsigned int)totalBytes)
    {
        printf("Unable to read file\n");
        free(bytesStr);
        return;
    }

    unsigned long byteSum = 0;
    for (long i = 0; i < totalBytes; i++)
        byteSum += (unsigned long)bytesStr[i];

    printf("%lf\n", (byteSum / 100.0));

    free(bytesStr);
    fclose(dataFile);
}