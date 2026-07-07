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

    if (fwrite(&header->status, sizeof(char), 1, dataFile) != 1 ||
        fwrite(&header->top, sizeof(int), 1, dataFile) != 1 ||
        fwrite(&header->nextRRN, sizeof(int), 1, dataFile) != 1 ||
        fwrite(&header->numStations, sizeof(int), 1, dataFile) != 1 ||
        fwrite(&header->numPairStations, sizeof(int), 1, dataFile) != 1)
        return FAILURE;

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
    if (!dataFile)
        return FAILURE;

    DataHeader *header = read_data_header(dataFile);
    if (!header)
        return FAILURE;

    Status status = FAILURE;

    char **seenStations = malloc(EXPECTED_SIZE * sizeof(char *));
    StationPair *seenPairs = malloc(EXPECTED_SIZE * sizeof(StationPair));
    int numStations = 0, numPairStations = 0;

    if (!seenStations || !seenPairs)
        goto cleanup;

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

    if (write_data_header(dataFile, header) == FAILURE)
        goto cleanup;

    status = SUCCESS;

cleanup:
    if (seenStations)
    {
        for (int i = 0; i < numStations; i++)
            free(seenStations[i]);
        free(seenStations);
    }
    free(seenPairs);
    free(header);

    return status;
}

// File Functions

Status write_data_file(char *inputPath, char *outputPath)
{
    if (!inputPath || !outputPath)
        return FAILURE;

    FILE *csvFile = fopen(inputPath, "r");
    FILE *binFile = fopen(outputPath, "wb+"); // wb+ because the "update_header_count" function needs to write
    DataHeader *fileHeader = create_data_header();

    Status status = FAILURE;

    if (!csvFile || !binFile || !fileHeader)
        goto cleanup;

    change_status(binFile, STATUS_INCONSISTENT);

    char buffer[BUF_SIZE]; // BUF_SIZE from types.h
    int numData = 0;

    // two different things; write_data_header and skip the column definition of the csv
    if (write_data_header(binFile, fileHeader) == FAILURE ||
        !fgets(buffer, BUF_SIZE, csvFile))
        goto cleanup;

    while (fgets(buffer, sizeof(buffer), csvFile))
    {
        Register *newRegister = parse_register(buffer);
        if (!newRegister)
            continue;

        newRegister->removed = RECORD_ACTIVE;
        newRegister->next = -1;

        if (write_register(binFile, newRegister) == FAILURE)
        {
            destroy_register(&newRegister);
            goto cleanup;
        }

        numData++;

        destroy_register(&newRegister);
    }

    fileHeader->nextRRN = numData;

    if (write_data_header(binFile, fileHeader) == FAILURE)
        goto cleanup;

    // could pass the fileHeader to it, but I prefered to make the function read the header by itself
    if (update_data_header_count(binFile) == FAILURE)
        goto cleanup;

    change_status(binFile, STATUS_CONSISTENT);

    status = SUCCESS;

cleanup:
    CLOSE_FILES(csvFile, binFile);
    free(fileHeader);

    return status;
}

Status print_all_data(char *dataPath)
{
    if (!dataPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb");
    Status status = FAILURE;

    if (!dataFile || check_header_consistency(dataFile) == FAILURE)
        goto cleanup;

    if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
        goto cleanup;

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

    status = SUCCESS;

cleanup:
    CLOSE_FILES(dataFile);

    return status;
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
    int pairIterations = 0;
    SearchField *filters = get_all_search_fields(&pairIterations);

    if (!filters)
        return NULL;

    DataHeader *header = read_data_header(dataFile);
    if (!header)
        goto err_cleanup_filters;

    if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
        goto err_cleanup_header;

    *numFound = 0;
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
    int capacity = header->nextRRN > 0 ? header->nextRRN : 1; // capacity is doubled and array is reallocated when necessary

    // Will have a size of 1 if the search is by stationCode, since we need to break.
    if (searchByStationCode)
        stationRRNList = calloc(1, sizeof(int));
    else
        stationRRNList = calloc(capacity, sizeof(int));

    if (!stationRRNList)
        goto err_cleanup_header;

    Register *filteredRegister = NULL;
    int currentRRN = 0;

    // get each register that matches the filter, save its RRN to the array.
    while ((filteredRegister = check_register_field_search(dataFile, filters, pairIterations, &currentRRN)))
    {
        stationRRNList[*numFound] = currentRRN;

        (*numFound)++;
        currentRRN++;

        destroy_register(&filteredRegister);

        if (searchByStationCode)
            break;
    }

    // if no matching registers were found, return NULL
    if ((*numFound) == 0)
        goto err_cleanup_all;

    free(header);
    free(filters);

    // caller needs to free it
    return stationRRNList;

err_cleanup_all:
    free(stationRRNList);
err_cleanup_header:
    free(header);
err_cleanup_filters:
    free(filters);

    return NULL;
}

Status print_all_data_where(char *dataPath, int iterations)
{
    if (!dataPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb");
    Status status = FAILURE;

    if (!dataFile || check_header_consistency(dataFile) == FAILURE)
        goto cleanup;

    int *filteredRRNs = NULL;
    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        filteredRRNs = filter_data_rrn(dataFile, &numFound);

        if (filteredRRNs)
        {
            for (int j = 0; j < numFound; j++)
            {
                if (fseek(dataFile, HEADER_SIZE + (REGISTER_SIZE * filteredRRNs[j]), SEEK_SET))
                    goto cleanup_filtered;

                Register *printedRegister = read_register(dataFile);
                if (!printedRegister)
                    goto cleanup_filtered;

                print_register(printedRegister);
                destroy_register(&printedRegister);
            }
        }
        else
            printf("Registro inexistente.\n");

        printf("\n");
        free(filteredRRNs);
        filteredRRNs = NULL;
    }

    status = SUCCESS;

cleanup_filtered:
    free(filteredRRNs);
cleanup:
    CLOSE_FILES(dataFile);

    return status;
}

Status delete_all_data_where(char *dataPath, int iterations)
{
    if (!dataPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb+");
    Status status = FAILURE;

    if (!dataFile || check_header_consistency(dataFile) == FAILURE)
        goto cleanup;

    change_status(dataFile, STATUS_INCONSISTENT);

    int *filteredRRNs = NULL;
    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        filteredRRNs = filter_data_rrn(dataFile, &numFound);

        if (filteredRRNs)
        {
            for (int j = 0; j < numFound; j++)
            {
                if (remove_register(dataFile, filteredRRNs[j]) == FAILURE)
                    goto cleanup_filtered;
            }
        }

        free(filteredRRNs);
        filteredRRNs = NULL;
    }

    if (update_data_header_count(dataFile) == FAILURE)
        goto cleanup;

    change_status(dataFile, STATUS_CONSISTENT);

    status = SUCCESS;

cleanup_filtered:
    free(filteredRRNs);
cleanup:
    CLOSE_FILES(dataFile);

    return status;
}

Status insert_data(char *dataPath, int iterations)
{
    if (!dataPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb+");
    Status status = FAILURE;

    if (!dataFile)
        return FAILURE;

    if (check_header_consistency(dataFile) == FAILURE)
        goto cleanup;

    DataHeader *header = read_data_header(dataFile);
    if (!header)
        goto cleanup_header;

    change_status(dataFile, STATUS_INCONSISTENT);
    header->status = STATUS_INCONSISTENT;

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

    if (update_data_header_count(dataFile) == FAILURE)
        goto cleanup_header;

    change_status(dataFile, STATUS_CONSISTENT);

    status = SUCCESS;

cleanup_header:
    free(header);
cleanup:
    CLOSE_FILES(dataFile);

    return status;
}

Status update_data_where(char *dataPath, int iterations)
{
    if (!dataPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb+");
    Status status = FAILURE;

    if (!dataFile || check_header_consistency(dataFile) == FAILURE)
        goto cleanup;

    change_status(dataFile, STATUS_INCONSISTENT);

    Register *updatedRegister = NULL;
    int *filteredRRNs = NULL;
    SearchField *updateFilters = NULL;
    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        filteredRRNs = filter_data_rrn(dataFile, &numFound);

        // not really a search field in this case, but can be repurposed.
        int updatePairIterations = 0;
        updateFilters = get_all_search_fields(&updatePairIterations);

        if (filteredRRNs && updateFilters)
        {
            for (int j = 0; j < numFound; j++)
            {
                if (fseek(dataFile, HEADER_SIZE + (REGISTER_SIZE * filteredRRNs[j]), SEEK_SET))
                    goto cleanup_loop;

                updatedRegister = read_register(dataFile);
                if (!updatedRegister)
                    goto cleanup_loop;

                if (update_register(dataFile, updatedRegister, updateFilters, updatePairIterations) == FAILURE)
                    goto cleanup_loop;

                destroy_register(&updatedRegister);
                updatedRegister = NULL;
            }
        }

        free(filteredRRNs);
        free(updateFilters);
        filteredRRNs = NULL;
        updateFilters = NULL;
    }

    change_status(dataFile, STATUS_CONSISTENT);

    status = SUCCESS;

cleanup_loop:
    if (updatedRegister)
        destroy_register(&updatedRegister);
    free(filteredRRNs);
    free(updateFilters);
cleanup:
    CLOSE_FILES(dataFile);

    return status;
}

Status select_join(char *sourcePath, char *joinPath)
{
    if (!sourcePath || !joinPath)
        return FAILURE;

    FILE *sourceFile = fopen(sourcePath, "rb");
    FILE *joinFile = fopen(joinPath, "rb");
    Status status = FAILURE;

    if (!sourceFile || !joinFile)
        goto cleanup;

    if (check_header_consistency(sourceFile) == FAILURE ||
        check_header_consistency(joinFile) == FAILURE)
        goto cleanup;

    if (fseek(sourceFile, HEADER_SIZE, SEEK_SET))
        goto cleanup;

    // we start reading each register by the start of the sourceFile
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
            goto cleanup;
        }

        Register *joinRegister = NULL;
        // unlike select_join_index, we read each register from joinFile until finding a match.
        while ((joinRegister = read_register(joinFile)))
        {
            // If the register in the join file is not removed and both sourceRegister and joinRegister meet the condition
            if (joinRegister->removed == RECORD_ACTIVE &&
                sourceRegister->nextStationCode == joinRegister->stationCode)
            {
                // print all requested values
                printf("%d %s %s %d %s\n",
                       sourceRegister->stationCode,
                       sourceRegister->stationName,
                       sourceRegister->lineName,
                       sourceRegister->nextStationCode,
                       joinRegister->stationName);

                destroy_register(&joinRegister);
                break;
            }

            destroy_register(&joinRegister);
        }

        destroy_register(&sourceRegister);
    }

    status = SUCCESS;

cleanup:
    CLOSE_FILES(sourceFile, joinFile);

    return status;
}

/**
 * @brief Static function used in order_by's qsort() to compare the stationCode of two different registers
 *
 * @param a First register
 * @param b Second register
 * @return >0 if the stationCode of a is greater than b, <0 if the stationCode of b is greater than a. 0 if they're equal.
 */
static int compare_registers_station(const void *a, const void *b)
{
    Register *regA = *(Register **)a;
    Register *regB = *(Register **)b;

    if (regA->stationCode > regB->stationCode)
        return 1;
    if (regA->stationCode < regB->stationCode)
        return -1;

    return 0;
}

/**
 * @brief Static function used in order_by's qsort() to compare the nextStationCode of two different registers
 *
 * @param a First register
 * @param b Second register
 * @return >0 if the nextStationCode of a is greater than b, <0 if the nextStationCode of b is greater than a. 0 if they're equal.
 */
static int compare_registers_next(const void *a, const void *b)
{
    Register *regA = *(Register **)a;
    Register *regB = *(Register **)b;

    if (regA->nextStationCode > regB->nextStationCode)
        return 1;
    if (regA->nextStationCode < regB->nextStationCode)
        return -1;

    return 0;
}

Status order_by(char *dataPath, char *field, char *orderedPath)
{
    if (!dataPath || !field || !orderedPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb");
    Status status = FAILURE;
    int idx = 0;

    if (!dataFile)
        return FAILURE;

    if (check_header_consistency(dataFile) == FAILURE)
        goto cleanup_data_file;

    DataHeader *header = read_data_header(dataFile);
    if (!header)
        goto cleanup_data_file;

    // Dynamically allocated array that'll store pointers to all registers. The maximum amount of registers will be equal to header's nextRRN.
    Register **savedRegisters = malloc(sizeof(Register *) * header->nextRRN);
    if (!savedRegisters)
        goto cleanup_header;

    if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
        goto cleanup_registers;

    // reading all registers from the start
    Register *currentRegister = NULL;
    while (idx < header->nextRRN && (currentRegister = read_register(dataFile)))
    {
        if (currentRegister->removed == RECORD_REMOVED)
        {
            destroy_register(&currentRegister);
            continue;
        }

        // if the register is not removed, save it to the array.
        savedRegisters[idx++] = currentRegister;
    }

    // close the data file since we are not using it anymore, and to deal with the case where dataPath == orderedPath
    fclose(dataFile);
    dataFile = NULL;

    // 0 = codEstacao, 1 = codProxEstacao
    int fieldValue = !strcmp(field, "codEstacao") ? 0 : 1;

    // if the field is codEstacao
    if (fieldValue == 0)
        qsort(savedRegisters, idx, sizeof(Register *), compare_registers_station);
    else // if its codProxEstacao
        qsort(savedRegisters, idx, sizeof(Register *), compare_registers_next);

    FILE *orderedFile = fopen(orderedPath, "wb");

    if (!orderedFile)
        goto cleanup_registers;

    // we don't order removed registers, so the top should be -1, nextRRN is also set to idx. This is only important when the dataFile has removed registers.
    header->status = STATUS_INCONSISTENT;
    header->top = -1;
    header->nextRRN = idx;

    if (write_data_header(orderedFile, header) == FAILURE)
        goto cleanup_all;

    if (fseek(orderedFile, HEADER_SIZE, SEEK_SET))
        goto cleanup_all;

    // write all registers and destroy their allocated pointer right after.
    for (int i = 0; i < idx; i++)
    {
        if (write_register(orderedFile, savedRegisters[i]) == FAILURE)
            goto cleanup_all;

        destroy_register(&savedRegisters[i]);
        savedRegisters[i] = NULL;
    }

    change_status(orderedFile, STATUS_CONSISTENT);

    status = SUCCESS;

cleanup_all:
    CLOSE_FILES(orderedFile);
cleanup_registers:
    for (int i = 0; i < idx; i++)
        if (savedRegisters[i])
            destroy_register(&savedRegisters[i]);

    free(savedRegisters);
cleanup_header:
    free(header);
cleanup_data_file:
    CLOSE_FILES(dataFile);

    return status;
}

Status select_join_order_by(char *sourcePath, char *joinPath)
{
    if (!sourcePath || !joinPath)
        return FAILURE;

    // order both files, first by "codProxEstacao", second one by "codEstacao", as specified in the assignment pdf.
    if (order_by(sourcePath, "codProxEstacao", sourcePath) == FAILURE ||
        order_by(joinPath, "codEstacao", joinPath) == FAILURE)
        return FAILURE;

    // joins both
    return select_join(sourcePath, joinPath);
}