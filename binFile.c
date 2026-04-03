#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "register.h"
#include "types.h"
#include "binFile.h"

// Header functions

/**
 * @brief Creates a hearder struct and sets it with default values
 *
 * @return Header* pointer to the dinamically alocated Header
 */
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

/**
 * @brief writes Header to a file
 *
 * @param file File that the header will be written to
 * @param header header to be written
 *
 * @return int returns HEADER_SUCESS if sucesseful or HEADER_FAILURE if not sucesseful
 */
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

/**
 * @brief writes a '0' char to the beggining of a file
 *
 * @param file file that the '0' will be written
 */
void status0(FILE *file)
{
    unsigned char status = '0';
    fseek(file, 0, SEEK_SET);
    fwrite(&status, sizeof(unsigned char), 1, file);

    fflush(file);
}

/**
 * @brief writes a '1' char to the beggining of a file
 *
 * @param file file that the '1' will be written
 */
void status1(FILE *file)
{
    unsigned char status = '1';
    fseek(file, 0, SEEK_SET);
    fwrite(&status, sizeof(unsigned char), 1, file);

    fflush(file);
}

//

/**
 * @brief writes a binary file with registers from a input .csv file
 *
 * @param inputFile Open input .csv file in "r" mode
 * @param outputFile Open output binary file in "wb+" mode
 *
 * @return DATA_SUCESS if sucesseful or DATA_FAILURE if unsucesseful
 */
DataStatus write_bin_file(FILE *inputFile, FILE *outputFile)
{
    if (!inputFile || !outputFile)
        return DATA_FAILURE;

    Header *tempHeader = create_header();
    char **seenStations = malloc(EXPECTED_SIZE * sizeof(char *));
    Pair *seenPairs = malloc(EXPECTED_SIZE * sizeof(Pair));

    char buffer[BUF_SIZE];

    int numData = 0, numStations = 0, numPairStations = 0;

    if (!tempHeader || !seenStations || !seenPairs || write_header(outputFile, tempHeader) == DATA_FAILURE || !fgets(buffer, BUF_SIZE, inputFile))
    {
        free(tempHeader);
        free(seenStations);
        free(seenPairs);
        return DATA_FAILURE;
    }

    while (fgets(buffer, sizeof(buffer), inputFile))
    {
        Register *newRegister = parse_register(buffer);
        if (!newRegister)
            continue;

        newRegister->removed = '0';
        newRegister->next = tempHeader->top;

        if (newRegister->stationName)
        {
            int foundName = 0;
            for (int i = 0; i < numStations; i++)
            {
                if (strcmp(seenStations[i], newRegister->stationName) == 0)
                {
                    foundName = 1;
                    break;
                }
            }

            if (!foundName)
                seenStations[numStations++] = strdup(newRegister->stationName);
        }

        if (newRegister->nextStationCode != -1)
        {
            int foundPair = 0;
            // impossibilitating cases like (1, 2) != (2, 1)
            int first = (newRegister->stationCode < newRegister->nextStationCode) ? newRegister->stationCode : newRegister->nextStationCode;
            int scnd = (newRegister->stationCode < newRegister->nextStationCode) ? newRegister->nextStationCode : newRegister->stationCode;
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

        write_register(outputFile, newRegister);
        numData++;

        destroy_register(&newRegister);
    }

    tempHeader->nextRRN = numData;
    tempHeader->numStations = numStations;
    tempHeader->numPairStations = numPairStations;

    write_header(outputFile, tempHeader);

    for (int i = 0; i < numStations; i++)
        free(seenStations[i]);
    free(seenStations);
    free(seenPairs);
    free(tempHeader);

    return DATA_SUCCESS;
}

// printing related

/**
 * @brief prints all registers from a binary file
 *
 * @param binFile Open binary file
 *
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus print_all_data(FILE *binFile)
{
    if (!binFile)
        return DATA_FAILURE;

    if (fseek(binFile, HEADER_SIZE, SEEK_SET))
        return DATA_FAILURE;

    Register *tmpRegister;
    while ((tmpRegister = read_register(binFile)))
    {
        if (tmpRegister->removed != '1')
            print_register(tmpRegister);

        destroy_register(&tmpRegister);
    }

    return DATA_SUCCESS;
}

/**
 * @brief prints all registers that meets the filters requirements
 *
 * @param binFile Open binary file
 * @param iterations Number of searches
 *
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus print_all_data_where(FILE *binFile, int iterations)
{
    if (!binFile)
        return DATA_FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        if (fseek(binFile, HEADER_SIZE, SEEK_SET))
            return DATA_FAILURE;

        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        Register *tmpRegister = NULL;
        int isMatch = 1;

        while ((tmpRegister = check_register_field_search(binFile, filters, pairIterations, &isMatch)))
        {
            if (isMatch && tmpRegister)
                print_register(tmpRegister);

            destroy_register(&tmpRegister);
        }

        free(filters);
    }

    return DATA_SUCCESS;
}

// delete

/**
 * @brief deletes all registers that meets the filters requirements
 */
DataStatus delete_all_data_where(FILE *binFile, int iterations)
{
    if (!binFile)
        return DATA_FAILURE;

    for (int i = 0; i < iterations; i++)
    {
        if (fseek(binFile, HEADER_SIZE, SEEK_SET))
            return DATA_FAILURE;

        int pairIterations = 0;
        SearchField *filters = get_all_search_fields(&pairIterations);

        Register *tmpRegister = NULL;
        int isMatch = 1;
        while ((tmpRegister = check_register_field_search(binFile, filters, pairIterations, &isMatch)))
        {
            if (isMatch && tmpRegister)
            {
                remove_register(binFile);
                fseek(binFile, REGISTER_SIZE - sizeof(char) - sizeof(int), SEEK_CUR);
            }

            destroy_register(&tmpRegister);
        }

        free(filters);
    }

    return DATA_SUCCESS;
}

//

/**
 * @brief Prints a checksum to validate a binary file
 *
 * @param fileName String containing the name of the binary file
 */
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