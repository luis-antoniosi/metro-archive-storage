#include "header.h"
#include <stdlib.h>

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

void status0(FILE *file){
    unsigned char status = '0';
    fseek(file, 0, SEEK_SET);
    fwrite(&status, sizeof(unsigned char), 1, file);

    fflush(file);
}
void status1(FILE *file){
    unsigned char status = '1';
    fseek(file, 0, SEEK_SET);
    fwrite(&status, sizeof(unsigned char), 1, file);

    fflush(file);
}