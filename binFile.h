#ifndef BINFILE_H
#define BINFILE_H

#include <stdio.h>
#include "types.h"
#include "register.h"

// Header

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

// Write file
/**
 * @brief writes a binary file with registers from a input .csv file
 *
 * @param inputFile Open input .csv file in "r" mode
 * @param outputFile Open output binary file in "wb+" mode
 *
 * @return DATA_SUCESS if sucesseful or DATA_FAILURE if unsucesseful
 */
DataStatus write_bin_file(FILE *inputFile, FILE *outputFile);

// Print
/**
 * @brief prints all registers from a binary file
 *
 * @param binFile Open binary file
 *
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus print_all_data(FILE *binFile);

/**
 * @brief prints all registers that meets the filters requirements
 *
 * @param binFile Open binary file
 * @param iterations Number of searches
 *
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus print_all_data_where(FILE *binFile, int iterations);

// Delete
/**
 * @brief deletes all registers that meets the filters requirements
 */
DataStatus delete_all_data_where(FILE *binFile, int iterations);

/**
 * @brief Inserts multiple registers in a binary file
 * 
 * @param binFile Pointer to the open binary file
 * @param iterations number of insertions
 * 
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus insert_data(FILE *binFile, int iterations);

/**
 * @brief Searchs for records that matches a criteria and updates them
 * 
 * @param binFile Open binary file
 * @param iterations number of update operations
 * 
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus update_data_where(FILE *binFile, int iterations);

// Print binary
/**
 * @brief Prints a checksum to validate a binary file
 *
 * @param fileName String containing the name of the binary file
 */
void binary_on_screen(char *fileName);

#endif