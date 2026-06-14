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
    char buffer[BUF_SIZE];  // BUF_SIZE from types.h
    int numData = 0;

    // essentially this if does three different things, but i didnt want to write 3 different ifs
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

    write_header(outputFile, fileHeader);
    free(fileHeader);

    // could pass the fileHeader to it, but I prefered to make the function read the header by itself
    if (update_header_count(outputFile) == FAILURE)
        return FAILURE;

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
// todo: combine this with select?
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
    Header *currHeader = read_header(binFile);
    if (!currHeader)
        return FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        Register *currentReg = input_register();

        if (currentReg)
        {
            insert_register(binFile, currentReg, currHeader);
            destroy_register(&currentReg);
        }
    }

    write_header(binFile, currHeader);
    free(currHeader);

    if (update_header_count(binFile) == FAILURE)
        return FAILURE;

    change_status(binFile, STATUS_CONSISTENT);

    return SUCCESS;
}

// TODO: Combine this with select too?
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

// part 2; index related

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

    change_status(indexFile, STATUS_CONSISTENT); // TODO: change this and other functions to another .c

    return SUCCESS;
}

Status search_with_index(FILE *registerFile, FILE *indexFile, int iterations)
{
    if (!registerFile || !indexFile)
        return FAILURE;

    BTHeader *btHeader = read_btheader(indexFile);
    if (!btHeader)
        return FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        int stationCode = -1;
        for (int j = 0; j < pairIterations; j++)
        {
            if (strcmp(filters[j].name, "codEstacao") == 0)
            {
                stationCode = atoi(filters[j].value);
                break;
            }
        }

        int anyMatches = 0;
        if (stationCode != -1)
        {
            int byteOffset = search_key(indexFile, btHeader, stationCode);
            if (byteOffset != -1)
            {
                fseek(registerFile, byteOffset, SEEK_SET);
                Register *reg = read_register(registerFile);
                if (reg && reg->removed != '1')
                {
                    print_register(reg);
                    anyMatches = 1;
                }
                destroy_register(&reg);
            }
        }
        else
        {
            fseek(registerFile, HEADER_SIZE, SEEK_SET);
            Register *reg = NULL;
            while ((reg = check_register_field_search(registerFile, filters, pairIterations)))
            {
                print_register(reg);
                anyMatches = 1;
                destroy_register(&reg);
            }
        }

        if (!anyMatches)
            printf("Registro inexistente.\n");

        printf("\n");
        free(filters);
    }

    return SUCCESS;
}

// todo: not happy with the current state of this function
Status insert_index(FILE *registerFile, FILE *indexFile, int iterations)
{
    if (!registerFile || !indexFile)
        return FAILURE;

    change_status(indexFile, STATUS_INCONSISTENT);

    Header *dataHeader = read_header(registerFile);
    BTHeader *btHeader = read_btheader(indexFile);
    if (!dataHeader || !btHeader)
    {
        free(dataHeader);
        free(btHeader);
        return FAILURE;
    }

    for (int i = 0; i < iterations; i++)
    {
        Register *currentReg = input_register();

        if (currentReg)
        {
            if (search_key(indexFile, btHeader, currentReg->stationCode) == -1)
            {
                int rrn = (dataHeader->top != -1) ? dataHeader->top : dataHeader->nextRRN;

                insert_register(registerFile, currentReg, dataHeader);
                insert_key(indexFile, btHeader, (BTKey){currentReg->stationCode, HEADER_SIZE + (rrn * REGISTER_SIZE)});
            }

            destroy_register(&currentReg);
        }
    }

    write_header(registerFile, dataHeader);
    write_btheader(indexFile, btHeader);
    free(dataHeader);
    free(btHeader);

    if (update_header_count(registerFile) == FAILURE)
        return FAILURE;

    change_status(indexFile, STATUS_CONSISTENT);

    return SUCCESS;
}

Status delete_index(FILE *registerFile, FILE *indexFile, int iterations)
{
    if (!registerFile || !indexFile)
        return FAILURE;

    change_status(registerFile, STATUS_INCONSISTENT);
    change_status(indexFile, STATUS_INCONSISTENT);

    BTHeader *btHeader = read_btheader(indexFile);
    if (!btHeader)
    {
        free(btHeader);
        return FAILURE;
    }

    for (int i = 0; i < iterations; i++)
    {
        fseek(registerFile, HEADER_SIZE, SEEK_SET);

        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        int stationCode = -1;
        for (int j = 0; j < pairIterations; j++)
        {
            if (strcmp(filters[j].name, "codEstacao") == 0)
            {
                stationCode = atoi(filters[j].value);
                break;
            }
        }

        if (stationCode != -1)
        {
            int byteOffset = search_key(indexFile, btHeader, stationCode);

            if (byteOffset != -1)
            {
                fseek(registerFile, byteOffset, SEEK_SET);

                Register *reg = read_register(registerFile);

                if (reg && reg->removed != '1')
                {
                    remove_register(registerFile);
                    remove_key(indexFile, btHeader, stationCode);
                }

                destroy_register(&reg);
            }
        }
        else
        {
            Register *reg = NULL;
            while ((reg = check_register_field_search(registerFile, filters, pairIterations)))
            {
                remove_register(registerFile);
                fseek(registerFile, REGISTER_SIZE - sizeof(char) - sizeof(int), SEEK_CUR);

                remove_key(indexFile, btHeader, reg->stationCode);
                destroy_register(&reg);
            }
        }

        free(filters);
    }

    write_btheader(indexFile, btHeader);

    if (update_header_count(registerFile) == FAILURE)
    {
        free(btHeader);
        return FAILURE;
    }

    change_status(registerFile, STATUS_CONSISTENT);
    change_status(indexFile, STATUS_CONSISTENT);

    free(btHeader);

    return SUCCESS;
}

// didn't really make our own, just copied and changed the variables' names
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