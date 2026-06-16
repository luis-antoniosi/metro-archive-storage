#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void change_status(FILE *dataFile, char status)
{
    fseek(dataFile, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, dataFile);
    fflush(dataFile);
}

void binary_on_screen(char *fileName)
{
    FILE *dataFile = NULL;

    if (!fileName || !(dataFile = fopen(fileName, "rb")))
        return;

    fseek(dataFile, 0, SEEK_END);
    long totalBytes = ftell(dataFile);

    fseek(dataFile, 0, SEEK_SET);
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