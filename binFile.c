#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "register.h"
#include "types.h"
#include "binFile.h"

// Header functions

Header *create_header()
{
    Header *header = malloc(sizeof(Header));

    if (!header)
        return NULL;

    header->status = '0';
    header->top = -1;
    header->nextRRN = 0;
    header->numStations = 0;
    header->numPairStations = 0;

    return header;
}

int write_header(FILE *file, Header *header)
{
    if (!file || !header)
        return HEADER_FAILURE;

    if (fseek(file, 0, SEEK_SET))
        return HEADER_FAILURE;

    fwrite(&header->status, sizeof(char), 1, file);
    fwrite(&header->top, sizeof(int), 1, file);
    fwrite(&header->nextRRN, sizeof(int), 1, file);
    fwrite(&header->numStations, sizeof(int), 1, file);
    fwrite(&header->numPairStations, sizeof(int), 1, file);

    return HEADER_SUCCESS;
}

void status0(FILE *file)
{
    unsigned char status = '0';
    fseek(file, 0, SEEK_SET);
    fwrite(&status, sizeof(unsigned char), 1, file);

    fflush(file);
}

void status1(FILE *file)
{
    unsigned char status = '1';
    fseek(file, 0, SEEK_SET);
    fwrite(&status, sizeof(unsigned char), 1, file);

    fflush(file);
}

//

DataStatus write_bin_file(FILE *inputFile, FILE *outputFile)
{
    if (!inputFile || !outputFile)
        return DATA_FAILURE;

    DataStatus result = DATA_FAILURE;
    Header *tempHeader = create_header();
    char **seenStations = malloc(EXPECTED_SIZE * sizeof(char *));
    Pair *seenPairs = malloc(EXPECTED_SIZE * sizeof(Pair));

    char buffer[BUF_SIZE];

    int numData = 0, numStations = 0, numPairStations = 0;

    // if (!tempHeader || !seenStations || !seenPairs || write_header(outputFile, tempHeader) == DATA_FAILURE || !fgets(buffer, BUF_SIZE, inputFile))
    // {
    //     free(tempHeader);
    //     free(seenStations);
    //     free(seenPairs);
    // }

    while (fgets(buffer, sizeof(buffer), inputFile))
    {
        Data *newData = tokenize(buffer);
        if (!newData)
            continue;

        newData->removed = '0';
        newData->next = tempHeader->top;

        if (newData->stationName)
        {
            int foundName = 0;
            for (int i = 0; i < numStations; i++)
            {
                if (strcmp(seenStations[i], newData->stationName) == 0)
                {
                    foundName = 1;
                    break;
                }
            }

            if (!foundName)
                seenStations[numStations++] = strdup(newData->stationName);
        }

        if (newData->nextStationCode != -1)
        {
            int foundPair = 0;
            // impossibilitating cases like (1, 2) !=    (2, 1)
            int first = (newData->stationCode < newData->nextStationCode) ? newData->stationCode : newData->nextStationCode;
            int scnd = (newData->stationCode < newData->nextStationCode) ? newData->nextStationCode : newData->stationCode;
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

        write_data(outputFile, newData);
        numData++;

        destroy_data(&newData);
    }

    tempHeader->nextRRN = numData;
    tempHeader->numStations = numStations;
    tempHeader->numPairStations = numPairStations;

    if (write_header(outputFile, tempHeader) == HEADER_FAILURE)
        goto clean;

    result = DATA_SUCCESS;

// there were some options:
// making another function that received three paramaters and repeating that each time something failed (boring),
// just repeating free() three times in a row for each failure (boring),
// big if (ugly),
// or... adding a goto (cool and fun)
clean:
    for (int i = 0; i < numStations; i++)
        free(seenStations[i]);
    free(seenStations);
    free(seenPairs);
    free(tempHeader);

    return result;
}

// printing related

DataStatus print_all_data(FILE *binFile)
{
    if (!binFile)
        return DATA_FAILURE;

    if (fseek(binFile, HEADER_SIZE, SEEK_SET))
        return DATA_FAILURE;

    Data *tmpData;
    while ((tmpData = read_data(binFile)))
    {
        if (tmpData->removed != '1')
            print_data(tmpData);

        destroy_data(&tmpData);
    }

    return DATA_SUCCESS;
}

DataStatus print_all_data_where(FILE *binFile, int iterations)
{
    if (!binFile)
        return DATA_FAILURE;

    if (fseek(binFile, HEADER_SIZE, SEEK_SET))
        return DATA_FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        Data *tmpData = NULL;
        int isMatch = 1;

        while ((tmpData = check_data_field_search(binFile, filters, pairIterations, &isMatch)))
        {
            if (isMatch && tmpData)
                print_data(tmpData);

            destroy_data(&tmpData);
        }

        free(filters);
    }

    return DATA_SUCCESS;
}

// delete

DataStatus delete_all_data_where(FILE *binFile, int iterations)
{
    if (!binFile)
        return DATA_FAILURE;

    if (fseek(binFile, HEADER_SIZE, SEEK_SET))
        return DATA_FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        Data *tmpData = NULL;
        int isMatch = 1;
        while ((tmpData = check_data_field_search(binFile, filters, pairIterations, &isMatch)))
        {
            if (isMatch && tmpData)
            {
                remove_data(binFile);
                fseek(binFile, DATA_SIZE - 1, SEEK_CUR);
            }

            destroy_data(&tmpData);
        }

        free(filters);
    }

    return DATA_SUCCESS;
}

//

void binary_on_screen(char *fileName)
{
    FILE *file = NULL;

    if (!fileName || !(file = fopen(fileName, "rb")))
        return;

    fseek(file, 0, SEEK_END);
    long totalBytes = ftell(file);

    fseek(file, 0, SEEK_SET);
    unsigned char *bytesStr = malloc(sizeof(unsigned char) * totalBytes);
    if (fread(bytesStr, 1, totalBytes, file) != (long unsigned int)totalBytes)
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
    fclose(file);
}
