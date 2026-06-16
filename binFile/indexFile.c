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

    write_index_header(indexFile, header);
    free(header);

    change_status(indexFile, STATUS_CONSISTENT);

    return SUCCESS;
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
            int byteOffset = search_index_key(indexFile, indexHeader, stationCode);
            if (byteOffset != -1)
            {
                fseek(dataFile, byteOffset, SEEK_SET);
                Register *reg = read_register(dataFile);
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
            fseek(dataFile, HEADER_SIZE, SEEK_SET);
            Register *reg = NULL;
            int currentRRN = 0;
            while ((reg = check_register_field_search(dataFile, filters, pairIterations, &currentRRN)))
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

    write_data_header(dataFile, dataHeader);
    write_index_header(indexFile, indexHeader);
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
        fseek(dataFile, HEADER_SIZE, SEEK_SET);

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
            int byteOffset = search_index_key(indexFile, indexHeader, stationCode);

            if (byteOffset != -1)
            {
                fseek(dataFile, byteOffset, SEEK_SET);

                Register *reg = read_register(dataFile);

                if (reg && reg->removed != '1')
                {
                    // TODO: Fix this
                    remove_register(dataFile, 0);
                    remove_index_key(indexFile, indexHeader, stationCode);
                }

                destroy_register(&reg);
            }
        }
        else
        {
            Register *reg = NULL;
            int currentRRN = 0;
            while ((reg = check_register_field_search(dataFile, filters, pairIterations, &currentRRN)))
            {
                remove_register(dataFile, 0);
                fseek(dataFile, REGISTER_SIZE - sizeof(char) - sizeof(int), SEEK_CUR);

                remove_index_key(indexFile, indexHeader, reg->stationCode);
                destroy_register(&reg);
            }
        }

        free(filters);
    }

    write_index_header(indexFile, indexHeader);

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