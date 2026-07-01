#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dataFile.h"

#include "register/register.h"
#include "register/search.h"
#include "register/modify.h"

#include "bTree/bTree.h"

#include "utils/utils.h"

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
        if (currentRegister->removed == RECORD_REMOVED)
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

// File Functions

Status write_data_file(FILE *inputFile, FILE *outputFile)
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

        newRegister->removed = RECORD_ACTIVE;
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
        if (currentReg->removed != RECORD_REMOVED)
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

// Helper function used in print_all_data_where, delete_all_data_where and update_data_where
/**
 * @brief Filters registers' RRNs based on SearchField filters doing a linear search
 *
 * @param dataFile File with all the registers
 * @param[out] numFound Number of found registers that match the filters
 * @return Pointer to an array of size numFound containing all RRNs. User must deallocate it.
 */
static int *filter_data_rrn(FILE *dataFile, int *numFound)
{
    if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
        return NULL;

    *numFound = 0;

    int pairIterations = 0;
    SearchField *filters = get_all_search_fields(&pairIterations);

    if (!filters)
        return NULL;

    int searchByStationCode = 0;
    for (int j = 0; j < pairIterations; j++)
    {
        if (strcmp(filters[j].name, "codEstacao") == 0)
        {
            searchByStationCode = 1;
            break;
        }
    }

    int *stationRRNList = NULL;
    int capacity = 4; // capacity is doubled and array is reallocated when necessary

    // Will have a size of 1 if the search is by stationCode, since we need to break.
    if (searchByStationCode)
        stationRRNList = calloc(1, sizeof(int));
    else
        stationRRNList = calloc(capacity, sizeof(int));

    if (!stationRRNList)
    {
        free(filters);
        free(stationRRNList);
        return NULL;
    }

    Register *filteredRegister = NULL;
    int currentRRN = 0;

    // get each register that matches the filter, save its RRN to the array.
    while ((filteredRegister = check_register_field_search(dataFile, filters, pairIterations, &currentRRN)))
    {
        if (*numFound >= capacity)
        {
            capacity *= 2;
            stationRRNList = realloc(stationRRNList, capacity * sizeof(int));
        }

        stationRRNList[*numFound] = currentRRN;

        (*numFound)++;
        currentRRN++;

        destroy_register(&filteredRegister);

        if (searchByStationCode)
            break;
    }

    // if no matching registers were found, return NULL
    if ((*numFound) == 0)
    {
        free(filters);
        free(stationRRNList);
        return NULL;
    }

    free(filters);

    // caller needs to free it
    return stationRRNList;
}

Status print_all_data_where(FILE *dataFile, int iterations)
{
    if (!dataFile)
        return FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        int *filteredRRNs = filter_data_rrn(dataFile, &numFound);

        if (filteredRRNs)
        {
            for (int j = 0; j < numFound; j++)
            {
                fseek(dataFile, HEADER_SIZE + (REGISTER_SIZE * filteredRRNs[j]), SEEK_SET);
                Register *printedRegister = read_register(dataFile);

                print_register(printedRegister);
                destroy_register(&printedRegister);
            }
        }
        else
            printf("Registro inexistente.\n");

        printf("\n");
        free(filteredRRNs);
    }

    return SUCCESS;
}

Status delete_all_data_where(FILE *dataFile, int iterations)
{
    if (!dataFile)
        return FAILURE;

    change_status(dataFile, STATUS_INCONSISTENT);

    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        int *filteredRRNs = filter_data_rrn(dataFile, &numFound);

        if (filteredRRNs)
        {
            for (int j = 0; j < numFound; j++)
                remove_register(dataFile, filteredRRNs[j]);
        }

        free(filteredRRNs);
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
    DataHeader *header = read_data_header(dataFile);
    if (!header)
        return FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        Register *currentReg = input_register();

        if (currentReg)
        {
            insert_register(dataFile, currentReg, header);
            destroy_register(&currentReg);
        }
    }

    write_data_header(dataFile, header);
    free(header);

    if (update_data_header_count(dataFile) == FAILURE)
        return FAILURE;

    change_status(dataFile, STATUS_CONSISTENT);

    return SUCCESS;
}

Status update_data_where(FILE *dataFile, int iterations)
{
    if (!dataFile)
        return FAILURE;

    change_status(dataFile, STATUS_INCONSISTENT);

    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        int *filteredRRNs = filter_data_rrn(dataFile, &numFound);

        // not really a search field in this case, but can be repurposed.
        int updatePairIterations = 0;
        SearchField *updateFilters = get_all_search_fields(&updatePairIterations);

        if (filteredRRNs)
        {
            for (int j = 0; j < numFound; j++)
            {
                fseek(dataFile, HEADER_SIZE + (REGISTER_SIZE * filteredRRNs[j]), SEEK_SET);
                Register *updatedRegister = read_register(dataFile);

                update_register(dataFile, updatedRegister, updateFilters, updatePairIterations);
                destroy_register(&updatedRegister);
            }
        }

        free(filteredRRNs);
        free(updateFilters);
    }

    change_status(dataFile, STATUS_CONSISTENT);

    return SUCCESS;
}

Status select_join(FILE *sourceFile, FILE *joinFile)
{
    if (!sourceFile || !joinFile)
        return FAILURE;

    if (fseek(sourceFile, HEADER_SIZE, SEEK_SET))
        return FAILURE;

    Register *sourceRegister = NULL;
    while ((sourceRegister = read_register(sourceFile)))
    {
        if (sourceRegister->removed == RECORD_REMOVED)
        {
            destroy_register(&sourceRegister);
            continue;
        }

        if (fseek(joinFile, HEADER_SIZE, SEEK_SET))
        {
            destroy_register(&sourceRegister);
            return FAILURE;
        }

        Register *joinRegister = NULL;
        while ((joinRegister = read_register(joinFile)))
        {
            if (joinRegister->removed == RECORD_ACTIVE &&
                sourceRegister->nextStationCode == joinRegister->stationCode)
            {
                printf("%d %s %s %d %s\n", 
                    sourceRegister->stationCode, 
                    sourceRegister->stationName, 
                    sourceRegister->lineName, 
                    sourceRegister->nextStationCode, 
                    joinRegister->stationName);
            }

            destroy_register(&joinRegister);
        }

        destroy_register(&sourceRegister);
    }

    return SUCCESS;
}