#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for strcmp

#include "indexFile.h"
#include "dataFile.h" // for write_data_header, read_data_header and update_data_header_count

#include "register/register.h" // for read_register, destroy_register and print_register. Register and SearchField types.
#include "register/modify.h"   // for input_register, insert_register and remove_register
#include "register/search.h"

#include "bTree/bTree.h"
#include "bTree/modify.h"

#include "utils/utils.h" // for change_status

// IndexHeader functions

IndexHeader *create_index_header()
{
    IndexHeader *header = malloc(sizeof(IndexHeader));

    if (!header)
        return NULL;

    header->status = STATUS_INCONSISTENT;
    header->rootNode = -1;
    header->top = -1;
    header->nextRRN = 0;
    header->numNodes = 0;

    return header;
}

Status write_index_header(FILE *indexFile, IndexHeader *header)
{
    if (!indexFile || !header)
        return FAILURE;

    if (fseek(indexFile, 0, SEEK_SET))
        return FAILURE;

    if (fwrite(&header->status, sizeof(char), 1, indexFile) != 1 ||
        fwrite(&header->rootNode, sizeof(int), 1, indexFile) != 1 ||
        fwrite(&header->top, sizeof(int), 1, indexFile) != 1 ||
        fwrite(&header->nextRRN, sizeof(int), 1, indexFile) != 1 ||
        fwrite(&header->numNodes, sizeof(int), 1, indexFile) != 1)
        return FAILURE;

    return SUCCESS;
}

IndexHeader *read_index_header(FILE *indexFile)
{
    if (!indexFile)
        return NULL;

    if (fseek(indexFile, 0, SEEK_SET))
        return NULL;

    IndexHeader *header = create_index_header();

    if (fread(&header->status, sizeof(char), 1, indexFile) != 1 ||
        fread(&header->rootNode, sizeof(int), 1, indexFile) != 1 ||
        fread(&header->top, sizeof(int), 1, indexFile) != 1 ||
        fread(&header->nextRRN, sizeof(int), 1, indexFile) != 1 ||
        fread(&header->numNodes, sizeof(int), 1, indexFile) != 1)
    {
        printf("Unable to read indexFile header.\n");
        free(header);
        return NULL;
    }

    return header;
}

// File functions

Status create_index(char *dataPath, char *indexPath)
{
    if (!dataPath || !indexPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb");
    FILE *indexFile = fopen(indexPath, "wb+"); // Needs wb+ because in insert_index_key we use allocate_page, which needs to read a page.
    IndexHeader *header = create_index_header();

    Status status = FAILURE;

    if (!dataFile || !indexFile || !header)
        goto cleanup;

    if (check_header_consistency(dataFile) == FAILURE)
        goto cleanup;

    if (write_index_header(indexFile, header) == FAILURE)
        goto cleanup;

    Register *reg = NULL;
    int rrn = 0;

    if (fseek(dataFile, HEADER_SIZE, SEEK_SET))
        goto cleanup;

    while ((reg = read_register(dataFile)))
    {
        if (reg->removed != RECORD_REMOVED)
        {
            IndexKey key = {reg->stationCode, HEADER_SIZE + (rrn * REGISTER_SIZE)};
            insert_index_key(indexFile, header, key);
        }

        destroy_register(&reg);
        rrn++;
    }

    if (write_index_header(indexFile, header) == FAILURE)
        goto cleanup;

    change_status(indexFile, STATUS_CONSISTENT);

    status = SUCCESS;

cleanup:
    CLOSE_FILES(dataFile, indexFile);
    free(header);
    return status;
}

// Helper function used in search_with_index and delete_index
/**
 * @brief Filters registers' RRNs based on SearchField filters using an indexFile.
 *
 * @param dataFile File with all the registers
 * @param indexFile File with all the indices
 * @param header Header of the index file
 * @param[out] numFound Number of found registers that match the filters
 * @return Pointer to an array of size numFound containing all RRNs. User must deallocate it.
 */
static int *filter_index_rrn(FILE *dataFile, FILE *indexFile, IndexHeader *header, int *numFound)
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

    // initially a size of 1, since we need to break in case the filter is just by a stationCode
    int *stationRRNList = calloc(1, sizeof(int));
    if (!stationRRNList)
        goto err_cleanup;

    Register *filteredRegister = NULL;
    // if the filter is by a stationCode
    if (stationCode != -1)
    {
        // use the index file to find the corresponding register, store its rrn in the array
        int byteOffset = search_index_key(indexFile, header, stationCode);

        if (byteOffset != -1)
        {
            if (fseek(dataFile, byteOffset, SEEK_SET))
                goto err_cleanup;

            // check if the found register meets all filters
            filteredRegister = read_register(dataFile);
            if (filteredRegister &&
                check_register_match(filteredRegister, filters, pairIterations) == SUCCESS)
            {
                stationRRNList[*numFound] = (byteOffset - HEADER_SIZE) / REGISTER_SIZE;
                (*numFound)++;
            }

            destroy_register(&filteredRegister);
        }
    }
    else
    {
        // if the filter doesn't use stationCode, do a linear search
        int currentRRN = 0;
        int capacity = 4; // initial capacity of 4, doubled when necessary

        // checking if reallocation works
        int *tmpList = realloc(stationRRNList, capacity * sizeof(int));

        if (!tmpList)
            goto err_cleanup;

        stationRRNList = tmpList;

        // get each register that matches the filter, save its RRN to the array.
        while ((filteredRegister = check_register_field_search(dataFile, filters, pairIterations, &currentRRN)))
        {
            if (*numFound >= capacity)
            {
                capacity *= 2;
                // checking if reallocation works
                tmpList = realloc(stationRRNList, capacity * sizeof(int));
                if (!tmpList)
                    goto err_cleanup;
                stationRRNList = tmpList;
            }

            stationRRNList[*numFound] = currentRRN;
            (*numFound)++;
            currentRRN++;

            destroy_register(&filteredRegister);
        }
    }

    // if no matching registers were found, return NULL
    if (*numFound == 0)
        goto err_cleanup;

    free(filters);

    // caller needs to free it
    return stationRRNList;

err_cleanup:
    free(stationRRNList);
    free(filters);

    return NULL;
}

Status search_with_index(char *dataPath, char *indexPath, int iterations)
{
    if (!dataPath || !indexPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb");
    FILE *indexFile = fopen(indexPath, "rb");

    Status status = FAILURE;

    if (!dataFile || !indexFile)
        goto cleanup_files;

    if (check_header_consistency(dataFile) == FAILURE ||
        check_header_consistency(indexFile) == FAILURE)
        goto cleanup_files;

    IndexHeader *indexHeader = read_index_header(indexFile);
    if (!indexHeader)
        goto cleanup_files;

    if (indexHeader->status == STATUS_INCONSISTENT)
        goto cleanup_header;

    int *filteredKeys = NULL;
    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        filteredKeys = filter_index_rrn(dataFile, indexFile, indexHeader, &numFound);

        if (filteredKeys)
        {
            for (int j = 0; j < numFound; j++)
            {
                // Go to its position using its byteOffset, read it and print it
                if (fseek(dataFile, HEADER_SIZE + (REGISTER_SIZE * filteredKeys[j]), SEEK_SET))
                    goto cleanup_all;

                Register *printedRegister = read_register(dataFile);
                if (!printedRegister)
                    goto cleanup_all;

                print_register(printedRegister);
                destroy_register(&printedRegister);
            }
        }
        else
            printf("Registro inexistente.\n");

        printf("\n");
        free(filteredKeys);
        filteredKeys = NULL;
    }

    status = SUCCESS;

cleanup_all:
    free(filteredKeys);
cleanup_header:
    free(indexHeader);
cleanup_files:
    CLOSE_FILES(dataFile, indexFile);
    return status;
}

Status insert_index(char *dataPath, char *indexPath, int iterations)
{
    if (!dataPath || !indexPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb+");
    FILE *indexFile = fopen(indexPath, "rb+");

    Status status = FAILURE;

    if (!dataFile || !indexFile)
        goto cleanup_files;

    if (check_header_consistency(dataFile) == FAILURE ||
        check_header_consistency(indexFile) == FAILURE)
        goto cleanup_files;

    change_status(dataFile, STATUS_INCONSISTENT);
    change_status(indexFile, STATUS_INCONSISTENT);

    DataHeader *dataHeader = read_data_header(dataFile);
    IndexHeader *indexHeader = read_index_header(indexFile);

    if (!dataHeader || !indexHeader)
        goto cleanup_all;

    for (int i = 0; i < iterations; i++)
    {
        Register *currentReg = input_register();

        if (currentReg)
        {
            if (search_index_key(indexFile, indexHeader, currentReg->stationCode) == -1)
            {
                int rrn = (dataHeader->top != -1) ? dataHeader->top : dataHeader->nextRRN;
                if (insert_register(dataFile, currentReg, dataHeader) == FAILURE)
                {
                    destroy_register(&currentReg);
                    goto cleanup_all;
                }

                IndexKey insertedKey = {currentReg->stationCode, HEADER_SIZE + (rrn * REGISTER_SIZE)};
                if (insert_index_key(indexFile, indexHeader, insertedKey) == FAILURE)
                {
                    destroy_register(&currentReg);
                    goto cleanup_all;
                }
            }

            destroy_register(&currentReg);
        }
    }

    if (write_data_header(dataFile, dataHeader) == FAILURE ||
        write_index_header(indexFile, indexHeader) == FAILURE)
        goto cleanup_all;

    if (update_data_header_count(dataFile) == FAILURE)
        goto cleanup_all;

    change_status(dataFile, STATUS_CONSISTENT);
    change_status(indexFile, STATUS_CONSISTENT);

    status = SUCCESS;

cleanup_all:
    free(dataHeader);
    free(indexHeader);
cleanup_files:
    CLOSE_FILES(dataFile, indexFile);
    return status;
}

static Status remove_register_and_index(FILE *dataFile, FILE *indexFile, IndexHeader *header, int rrn)
{
    if (fseek(dataFile, HEADER_SIZE + (rrn * REGISTER_SIZE), SEEK_SET))
        return FAILURE;

    Register *reg = read_register(dataFile);
    if (!reg)
        return FAILURE;

    if (remove_register(dataFile, rrn) == FAILURE ||
        remove_index_key(indexFile, header, reg->stationCode) == FAILURE)
    {
        destroy_register(&reg);
        return FAILURE;
    }

    destroy_register(&reg);
    return SUCCESS;
}

Status delete_index(char *dataPath, char *indexPath, int iterations)
{
    if (!dataPath || !indexPath)
        return FAILURE;

    FILE *dataFile = fopen(dataPath, "rb+");
    FILE *indexFile = fopen(indexPath, "rb+");

    Status status = FAILURE;

    if (!dataFile || !indexFile)
        goto cleanup_files;

    if (check_header_consistency(dataFile) == FAILURE ||
        check_header_consistency(indexFile) == FAILURE)
        goto cleanup_files;

    change_status(dataFile, STATUS_INCONSISTENT);
    change_status(indexFile, STATUS_INCONSISTENT);

    IndexHeader *indexHeader = read_index_header(indexFile);
    if (!indexHeader)
        goto cleanup_files;

    int *filteredKeys = NULL;
    for (int i = 0; i < iterations; i++)
    {
        int numFound = 0;
        filteredKeys = filter_index_rrn(dataFile, indexFile, indexHeader, &numFound);

        if (filteredKeys)
        {
            for (int j = 0; j < numFound; j++)
            {
                if (remove_register_and_index(dataFile, indexFile, indexHeader, filteredKeys[j]) == FAILURE)
                    goto cleanup_all;
            }

            free(filteredKeys);
            filteredKeys = NULL;
        }
    }

    if (write_index_header(indexFile, indexHeader) == FAILURE)
        goto cleanup_all;

    if (update_data_header_count(dataFile) == FAILURE)
        goto cleanup_all;

    change_status(dataFile, STATUS_CONSISTENT);
    change_status(indexFile, STATUS_CONSISTENT);

    status = SUCCESS;

cleanup_all:
    free(filteredKeys);
    free(indexHeader);
cleanup_files:
    CLOSE_FILES(dataFile, indexFile);
    return status;
}

Status select_join_index(char *sourcePath, char *joinPath, char *indexPath)
{
    if (!sourcePath || !joinPath || !indexPath)
        return FAILURE;

    FILE *sourceFile = fopen(sourcePath, "rb");
    FILE *joinFile = fopen(joinPath, "rb");
    FILE *indexFile = fopen(indexPath, "rb");

    Status status = FAILURE;

    if (!sourceFile || !joinFile || !indexFile)
        goto cleanup_files;

    if (check_header_consistency(sourceFile) == FAILURE ||
        check_header_consistency(joinFile) == FAILURE ||
        check_header_consistency(indexFile) == FAILURE)
        goto cleanup_files;

    if (fseek(sourceFile, HEADER_SIZE, SEEK_SET))
        goto cleanup_files;

    IndexHeader *indexHeader = read_index_header(indexFile);
    if (!indexHeader)
        goto cleanup_files;

    Register *sourceRegister = NULL;
    while ((sourceRegister = read_register(sourceFile)))
    {
        if (sourceRegister->removed == RECORD_REMOVED)
        {
            destroy_register(&sourceRegister);
            continue;
        }

        int byteOffset = search_index_key(indexFile, indexHeader, sourceRegister->nextStationCode);
        if (byteOffset != -1)
        {
            if (fseek(joinFile, byteOffset, SEEK_SET))
            {
                destroy_register(&sourceRegister);
                goto cleanup_all;
            }

            Register *joinRegister = read_register(joinFile);

            if (!joinRegister)
                goto cleanup_all;

            if (joinRegister->removed == RECORD_ACTIVE)
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

    status = SUCCESS;

cleanup_all:
    free(indexHeader);
cleanup_files:
    CLOSE_FILES(sourceFile, joinFile, indexFile);
    return status;
}