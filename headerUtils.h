#ifndef HEADER_UTILS_H
#define HEADER_UTILS_H

#include <stdio.h>
#include "types.h"

/**
 * @brief Creates a header struct and sets it with default values
 *
 * @return Header* pointer to the dinamically alocated Header
 */
Header *create_header();

/**
 * @brief writes Header to a file
 *
 * @param binFile File that the header will be written to
 * @param header header to be written
 *
 * @return int returns HEADER_SUCCESS if sucesseful or HEADER_FAILURE if not sucesseful
 */
HeaderStatus write_header(FILE *binFile, Header *header);

/**
 * @brief reads a Header from a file
 * 
 * @param binFile File that contains the header
 * 
 * @return header Populated header struct
 */
Header *read_header(FILE *binFile);

/**
 * @brief loads, updates and writes the Header with updated numStations and numPairStations
 * 
 * @param binFile Open binary file
 * 
 * @return HEADER_SUCESS or HEADER_FAILURE
 */
HeaderStatus update_header_count(FILE *binFile);

/**
 * @brief writes a specified status to the beginning of a file
 *
 * @param binFile file where the status will be written
 * @param status char for the status; STATUS_CONSISTENT ('1') or STATUS_INCONSISTENT ('0')
 */
void change_status(FILE *binFile, char status);

/**
 * @brief Recalculates the numStations and numPairStations count
 * 
 * @param binFile Pointer to the open binary file
 * @param header Pointer to the Header struct that will be updated
 * 
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus update_station_counts(FILE *binFile, Header *header);

#endif