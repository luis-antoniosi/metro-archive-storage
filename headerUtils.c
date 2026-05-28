#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "headerUtils.h"
#include "register.h"

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

HeaderStatus write_header(FILE *binFile, Header *header)
{
    if (!binFile || !header)
        return HEADER_FAILURE;

    if (fseek(binFile, 0, SEEK_SET))
        return HEADER_FAILURE;

    fwrite(&header->status, sizeof(char), 1, binFile);
    fwrite(&header->top, sizeof(int), 1, binFile);
    fwrite(&header->nextRRN, sizeof(int), 1, binFile);
    fwrite(&header->numStations, sizeof(int), 1, binFile);
    fwrite(&header->numPairStations, sizeof(int), 1, binFile);

    return HEADER_SUCCESS;
}

Header *read_header(FILE *binFile)
{
    if (!binFile)
        return NULL;

    if (fseek(binFile, 0, SEEK_SET))
        return NULL;

    Header *header = create_header();

    if (fread(&header->status, sizeof(char), 1, binFile) != 1 ||
        fread(&header->top, sizeof(int), 1, binFile) != 1 ||
        fread(&header->nextRRN, sizeof(int), 1, binFile) != 1 ||
        fread(&header->numStations, sizeof(int), 1, binFile) != 1 ||
        fread(&header->numPairStations, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read header.\n");
        free(header);
        return NULL;
    }

    return header;
}

HeaderStatus update_header_count(FILE *binFile)
{
    Header *fileHeader = read_header(binFile);
    if (!fileHeader)
        return HEADER_FAILURE;

    if (update_station_counts(binFile, fileHeader) == DATA_FAILURE)
    {
        free(fileHeader);
        return HEADER_FAILURE;
    }

    write_header(binFile, fileHeader);
    free(fileHeader);

    return HEADER_SUCCESS;
}

void change_status(FILE *binFile, char status)
{
    fseek(binFile, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, binFile);
    fflush(binFile);
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
    Register *currentRegister = NULL;
    while ((currentRegister = read_register(binFile)))
    {
        if (currentRegister->removed == '1')
        {
            destroy_register(&currentRegister);
            continue;
        }

        if (currentRegister->stationName)
        {
            int foundName = 0;
            for (int i = 0; i < numStations; i++)
            {
                if (strcmp(seenStations[i], currentRegister->stationName) == 0)
                {
                    foundName = 1;
                    break;
                }
            }

            if (!foundName)
                seenStations[numStations++] = strdup(currentRegister->stationName);
        }

        if (currentRegister->nextStationCode != -1)
        {
            int foundPair = 0;
            // impossibilitating cases like (1, 2) != (2, 1)
            int first = (currentRegister->stationCode < currentRegister->nextStationCode) ? currentRegister->stationCode : currentRegister->nextStationCode;
            int scnd = (currentRegister->stationCode < currentRegister->nextStationCode) ? currentRegister->nextStationCode : currentRegister->stationCode;
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

        destroy_register(&currentRegister);
    }

    header->numStations = numStations;
    header->numPairStations = numPairStations;

    for (int i = 0; i < numStations; i++)
        free(seenStations[i]);
    free(seenStations);
    free(seenPairs);


    if (write_header(binFile, header) == HEADER_FAILURE)
    {
        return DATA_FAILURE;
    }

    return DATA_SUCCESS;
}