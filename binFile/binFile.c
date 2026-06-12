#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "binFile.h"
#include "header/header.h"

#include "register/register.h"
#include "register/search.h"
#include "register/modify.h"

#include "bTree/bTree.h"

Status write_bin_file(FILE *inputFile, FILE *outputFile)
{
    if (!inputFile || !outputFile)
        return FAILURE;

    change_status(outputFile, STATUS_INCONSISTENT);

    Header *fileHeader = create_header();
    char buffer[BUF_SIZE];
    int numData = 0;

    // checks if header is not null, if it's not, write it to the bin file, and if that works, get the path of the .csv
    // if any of those fail, it frees the header and returns.
    if (!fileHeader || write_header(outputFile, fileHeader) || !fgets(buffer, BUF_SIZE, inputFile))
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

    // this function writes the header after getting the station count
    if (update_station_counts(outputFile, fileHeader) == FAILURE)
    {
        free(fileHeader);
        return FAILURE;
    }

    free(fileHeader);

    change_status(outputFile, STATUS_CONSISTENT);

    return SUCCESS;
}

// printing related

Status print_all_data(FILE *binFile)
{
    if (!binFile)
        return FAILURE;

    if (fseek(binFile, HEADER_SIZE, SEEK_SET))
        return FAILURE;

    Register *currentReg;
    int foundRegister = 0;
    while ((currentReg = read_register(binFile)))
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

Status print_all_data_where(FILE *binFile, int iterations)
{
    if (!binFile)
        return FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        if (fseek(binFile, HEADER_SIZE, SEEK_SET))
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

        while ((currentReg = check_register_field_search(binFile, filters, pairIterations)))
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
Status delete_all_data_where(FILE *binFile, int iterations)
{
    if (!binFile)
        return FAILURE;

    change_status(binFile, STATUS_INCONSISTENT);

    for (int i = 0; i < iterations; i++)
    {
        if (fseek(binFile, HEADER_SIZE, SEEK_SET))
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
        while ((currentReg = check_register_field_search(binFile, filters, pairIterations)))
        {
            remove_register(binFile);
            fseek(binFile, REGISTER_SIZE - sizeof(char) - sizeof(int), SEEK_CUR);

            destroy_register(&currentReg);

            // if primary key is used, break
            if (searchByStationCode)
            {
                break;
            }
        }

        free(filters);
    }

    if (update_header_count(binFile) == FAILURE)
        return FAILURE;

    change_status(binFile, STATUS_CONSISTENT);

    return SUCCESS;
}

Status insert_data(FILE *binFile, int iterations)
{
    if (!binFile)
        return FAILURE;

    change_status(binFile, STATUS_INCONSISTENT);

    for (int i = 0; i < iterations; i++)
    {
        Register *currentReg = NULL;
        currentReg = input_register();

        if (currentReg)
        {
            Header *currHeader = read_header(binFile);
            insert_register(binFile, currentReg, currHeader);

            write_header(binFile, currHeader);

            destroy_register(&currentReg);
            free(currHeader);
        }
    }

    if (update_header_count(binFile) == FAILURE)
        return FAILURE;

    change_status(binFile, STATUS_CONSISTENT);

    return SUCCESS;
}

Status update_data_where(FILE *binFile, int iterations)
{
    if (!binFile)
        return FAILURE;

    change_status(binFile, STATUS_INCONSISTENT);

    for (int i = 0; i < iterations; i++)
    {
        if (fseek(binFile, HEADER_SIZE, SEEK_SET))
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
        while ((currentReg = check_register_field_search(binFile, filters, pairIterations)))
        {
            update_register(binFile, currentReg, updateFilters, updatePairIterations);

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

    change_status(binFile, STATUS_CONSISTENT);

    return SUCCESS;
}

Status create_index(FILE *registerFile, FILE *indexFile)
{
    if (!registerFile || !indexFile)
        return FAILURE;

    BTHeader *btHeader = create_btheader();
    if (!btHeader || (write_btheader(indexFile, btHeader) == FAILURE))
    {
        free(btHeader);
        return FAILURE;
    }
    
    Register *reg;
    int rrn = 0;
    
    fseek(registerFile, HEADER_SIZE, SEEK_SET);
    while ((reg = read_register(registerFile)))
    {
        if (reg->removed != '1')
        {
            BTKey key = {reg->stationCode, HEADER_SIZE + (rrn * REGISTER_SIZE)};
            insert_key(indexFile, btHeader, key);
        }

        destroy_register(&reg);
        rrn++;
    }

    write_btheader(indexFile, btHeader);
    free(btHeader);

    return SUCCESS;
}

void binary_on_screen(char *fileName)
{
    FILE *binFile = NULL;

    if (!fileName || !(binFile = fopen(fileName, "rb")))
        return;

    fseek(binFile, 0, SEEK_END);
    long totalBytes = ftell(binFile);

    fseek(binFile, 0, SEEK_SET);
    unsigned char *bytesStr = malloc(sizeof(unsigned char) * totalBytes);
    if (fread(bytesStr, 1, totalBytes, binFile) != (long unsigned int)totalBytes)
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
    fclose(binFile);
}