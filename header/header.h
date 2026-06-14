#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include "types.h"

/**
 * @struct Header
 * @brief Represents the header record of the binary data file.
 */
typedef struct Header
{
    char status;            // STATUS_CONSISTENT or STATUS_INCONSISTENT; see types.h
    int top;                // byte offset of the last removed register, -1 if there are none
    int nextRRN;            // initially 0
    int numStations;        // initially 0
    int numPairStations;    // initially 0
} Header;

/**
 * @brief Creates a header struct and sets it with default values
 *
 * @return Header* pointer to the dynamically allocated Header
 */
Header *create_header();

/**
 * @brief Writes Header to a file
 *
 * @param binFile File that the header will be written to
 * @param header Header to be written
 *
 * @return SUCCESS or FAILURE
 */
Status write_header(FILE *binFile, Header *header);

/**
 * @brief Reads a Header from a file
 * 
 * @param binFile File that contains the header
 * 
 * @return Header* pointing to the the read header
 */
Header *read_header(FILE *binFile);

/**
 * @brief Loads, updates and writes the file's header with updated numStations and numPairStations
 * 
 * @param binFile Open binary file
 * 
 * @return SUCCESS or FAILURE
 */
Status update_header_count(FILE *binFile);

/**
 * @brief Writes a specified status to the beginning of a file
 *
 * @param binFile file where the status will be written
 * @param status char for the status; STATUS_CONSISTENT ('1') or STATUS_INCONSISTENT ('0')
 */
void change_status(FILE *binFile, char status);

#endif