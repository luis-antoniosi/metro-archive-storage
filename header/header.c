#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "register/register.h"

Header *create_header()
{
    Header *header = malloc(sizeof(Header));

    if (!header)
        return NULL;

    header->status = STATUS_INCONSISTENT;
    header->top = -1;
    header->nextRRN = 0;
    header->numStations = 0;
    header->numPairStations = 0;

    return header;
}

Status write_header(FILE *binFile, Header *header)
{
    if (!binFile || !header)
        return FAILURE;

    if (fseek(binFile, 0, SEEK_SET))
        return FAILURE;

    fwrite(&header->status, sizeof(char), 1, binFile);
    fwrite(&header->top, sizeof(int), 1, binFile);
    fwrite(&header->nextRRN, sizeof(int), 1, binFile);
    fwrite(&header->numStations, sizeof(int), 1, binFile);
    fwrite(&header->numPairStations, sizeof(int), 1, binFile);

    return SUCCESS;
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

Status update_header_count(FILE *binFile)
{
    Header *fileHeader = read_header(binFile);
    if (!fileHeader)
        return FAILURE;

    if (update_station_counts(binFile, fileHeader) == FAILURE)
    {
        free(fileHeader);
        return FAILURE;
    }

    write_header(binFile, fileHeader);
    free(fileHeader);

    return SUCCESS;
}

void change_status(FILE *binFile, char status)
{
    fseek(binFile, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, binFile);
    fflush(binFile);
}

Status update_station_counts(FILE *binFile, Header *header)
{
    fseek(binFile, HEADER_SIZE, SEEK_SET);

    char **seenStations = malloc(EXPECTED_SIZE * sizeof(char *));
    Pair *seenPairs = malloc(EXPECTED_SIZE * sizeof(Pair));

    if (!seenStations || !seenPairs || !header)
    {
        free(seenStations);
        free(seenPairs);
        return FAILURE;
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
            int isDuplicate = 0;
            for (int i = 0; i < numStations; i++)
            {
                if (strcmp(seenStations[i], currentRegister->stationName) == 0)
                {
                    isDuplicate = 1;
                    break;
                }
            }

            if (!isDuplicate)
            {
                seenStations[numStations++] = strdup(currentRegister->stationName);
            }
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

    if (write_header(binFile, header) == FAILURE)
    {
        return FAILURE;
    }

    return SUCCESS;
}