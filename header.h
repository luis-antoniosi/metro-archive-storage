#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>

typedef enum HeaderStatus
{
    HEADER_SUCCESS = 0,
    HEADER_FAILURE = 1
} HeaderStatus;

typedef struct Header Header;

Header *create_header();

int write_header(FILE *file, Header *header);

#endif