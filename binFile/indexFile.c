#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indexFile.h"
#include "dataFile.h"

#include "register/register.h"
#include "register/modify.h"
#include "register/search.h"

#include "bTree/bTree.h"
#include "bTree/modify.h"

#include "utils/utils.h"

// part 2; index related

Status create_index(FILE *dataFile, FILE *indexFile)
{
    if (!dataFile || !indexFile)
        return FAILURE;

    IndexHeader *header = create_index_header();
    if (!header || (write_index_header(indexFile, header) == FAILURE))
    {
        free(header);
        return FAILURE;
    }

    Register *reg;
    int rrn = 0;

    fseek(dataFile, HEADER_SIZE, SEEK_SET);
    while ((reg = read_register(dataFile)))
    {
        if (reg->removed != '1')
        {
            IndexKey key = {reg->stationCode, HEADER_SIZE + (rrn * REGISTER_SIZE)};
            insert_index_key(indexFile, header, key);
        }

        destroy_register(&reg);
        rrn++;
    }

    if (write_index_header(indexFile, header) == FAILURE)
    {
        free(header);
        return FAILURE;
    }

    free(header);

    change_status(indexFile, STATUS_CONSISTENT);

    return SUCCESS;
}

static int *filter_index_keys(FILE *dataFile, FILE *indexFile, IndexHeader *header, int *numFound)
{
    if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
        return NULL;

    *numFound = 0;

    int pairIterations = 0;
    SearchField *filters = get_all_search_fields(&pairIterations);

    if (!filters)
        return NULL;

    int stationCode = -1;
    for (int j = 0; j < pairIterations; j++)
    {
        if (strcmp(filters[j].name, "codEstacao") == 0)
        {
            stationCode = atoi(filters[j].value);
            break;
        }
    }

    int *stationCodeList = calloc(1, sizeof(int));
    if (!stationCodeList)
    {
        free(filters);
        return NULL;
    }

    if (stationCode != -1)
    {
        // use index file
        int byteOffset = search_index_key(indexFile, header, stationCode);
        if (byteOffset != -1)
        {
            stationCodeList[*numFound] = stationCode;
            (*numFound)++;
        }
    }
    else
    {
        // linear scan
        Register *filteredRegister = NULL;
        int currentRRN = 0;
        int capacity = 4;
        stationCodeList = realloc(stationCodeList, capacity * sizeof(int));

        while ((filteredRegister = check_register_field_search(dataFile, filters, pairIterations, &currentRRN)))
        {
            if (*numFound >= capacity)
            {
                capacity *= 2;
                stationCodeList = realloc(stationCodeList, capacity * sizeof(int));
            }

            stationCodeList[*numFound] = filteredRegister->stationCode;
            (*numFound)++;
            currentRRN++;

            destroy_register(&filteredRegister);
        }
    }

    free(filters);

    if (*numFound == 0)
    {
        free(stationCodeList);
        return NULL;
    }

    // caller needs to free it
    return stationCodeList;
}

Status search_with_index(FILE *dataFile, FILE *indexFile, int iterations)
{
    if (!dataFile || !indexFile)
        return FAILURE;

    IndexHeader *indexHeader = read_index_header(indexFile);
    if (!indexHeader)
        return FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        int *filteredKeys = filter_index_keys(dataFile, indexFile, indexHeader, &numFound);

        if (filteredKeys)
        {
            for (int j = 0; j < numFound; j++)
            {
                int byteOffset = search_index_key(indexFile, indexHeader, filteredKeys[j]);
                if (byteOffset != -1)
                {
                    fseek(dataFile, byteOffset, SEEK_SET);
                    Register *printedRegister = read_register(dataFile);

                    print_register(printedRegister);
                    destroy_register(&printedRegister);
                }
            }
        }
        else
            printf("Registro inexistente.\n");

        printf("\n");
        free(filteredKeys);
    }

    free(indexHeader);

    return SUCCESS;
}

// todo: not happy with the current state of this function
Status insert_index(FILE *dataFile, FILE *indexFile, int iterations)
{
    if (!dataFile || !indexFile)
        return FAILURE;

    change_status(indexFile, STATUS_INCONSISTENT);

    DataHeader *dataHeader = read_data_header(dataFile);
    IndexHeader *indexHeader = read_index_header(indexFile);
    if (!dataHeader || !indexHeader)
    {
        free(dataHeader);
        free(indexHeader);
        return FAILURE;
    }

    for (int i = 0; i < iterations; i++)
    {
        Register *currentReg = input_register();

        if (currentReg)
        {
            if (search_index_key(indexFile, indexHeader, currentReg->stationCode) == -1)
            {
                int rrn = (dataHeader->top != -1) ? dataHeader->top : dataHeader->nextRRN;

                insert_register(dataFile, currentReg, dataHeader);
                insert_index_key(indexFile, indexHeader, (IndexKey){currentReg->stationCode, HEADER_SIZE + (rrn * REGISTER_SIZE)});
            }

            destroy_register(&currentReg);
        }
    }

    if (write_data_header(dataFile, dataHeader) == FAILURE ||
        write_index_header(indexFile, indexHeader) == FAILURE)
    {
        free(dataHeader);
        free(indexHeader);
        return FAILURE;
    }

    free(dataHeader);
    free(indexHeader);

    if (update_data_header_count(dataFile) == FAILURE)
        return FAILURE;

    change_status(indexFile, STATUS_CONSISTENT);

    return SUCCESS;
}

Status delete_index(FILE *dataFile, FILE *indexFile, int iterations)
{
    if (!dataFile || !indexFile)
        return FAILURE;

    change_status(dataFile, STATUS_INCONSISTENT);
    change_status(indexFile, STATUS_INCONSISTENT);

    IndexHeader *indexHeader = read_index_header(indexFile);
    if (!indexHeader)
    {
        free(indexHeader);
        return FAILURE;
    }

    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        int *filteredKeys = filter_index_keys(dataFile, indexFile, indexHeader, &numFound);

        if (filteredKeys)
        {
            for (int j = 0; j < numFound; j++)
            {
                int byteOffset = search_index_key(indexFile, indexHeader, filteredKeys[j]);
                remove_register(dataFile, (byteOffset - HEADER_SIZE) / REGISTER_SIZE);

                remove_index_key(indexFile, indexHeader, filteredKeys[j]);
            }

            free(filteredKeys);
        }
    }

    if (write_index_header(indexFile, indexHeader) == FAILURE)
    {
        free(indexHeader);
        return FAILURE;
    }

    if (update_data_header_count(dataFile) == FAILURE)
    {
        free(indexHeader);
        return FAILURE;
    }

    change_status(dataFile, STATUS_CONSISTENT);
    change_status(indexFile, STATUS_CONSISTENT);

    free(indexHeader);

    return SUCCESS;
}