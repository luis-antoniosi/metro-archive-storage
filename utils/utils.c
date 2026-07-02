#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void change_status(FILE *dataFile, char status)
{
    if (fseek(dataFile, 0, SEEK_SET))
        return;

    fwrite(&status, sizeof(char), 1, dataFile);
    fflush(dataFile);
}

void binary_on_screen(char *fileName)
{
    FILE *dataFile = NULL;

    if (!fileName || !(dataFile = fopen(fileName, "rb")))
        return;

    if (fseek(dataFile, 0, SEEK_END))
        return;
    long totalBytes = ftell(dataFile);

    if (fseek(dataFile, 0, SEEK_SET))
        return;
        
    unsigned char *bytesStr = malloc(sizeof(unsigned char) * totalBytes);
    if (fread(bytesStr, 1, totalBytes, dataFile) != (long unsigned int)totalBytes)
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
    fclose(dataFile);
}

Status check_header_consistency(FILE *binFile)
{
    if (!binFile)
        return FAILURE;

    if (fseek(binFile, 0, SEEK_SET))
        return FAILURE;

    char status;
    if (fread(&status, sizeof(char), 1, binFile) != 1)
        return FAILURE;
    
    rewind(binFile); // going back to start

    return status == STATUS_CONSISTENT ? SUCCESS : FAILURE;
}