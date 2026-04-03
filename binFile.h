#ifndef BINFILE_H
#define BINFILE_H

#include <stdio.h>
#include "types.h"
#include "register.h"

// Header

/**
 * @brief Creates a hearder struct and sets it with default values
 *
 * @return Header* pointer to the dinamically alocated Header
 */
Header *create_header();

/**
 * @brief writes Header to a file
 *
 * @param file File that the header will be written to
 * @param header header to be written
 *
 * @return int returns HEADER_SUCESS if sucesseful or HEADER_FAILURE if not sucesseful
 */
int write_header(FILE *file, Header *header);

/**
 * @brief writes a '0' char to the beggining of a file
 *
 * @param file file that the '0' will be written
 */
void status0(FILE *file);

/**
 * @brief writes a '1' char to the beggining of a file
 *
 * @param file file that the '1' will be written
 */
void status1(FILE *file);

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

// Print binary
/**
 * @brief Prints a checksum to validate a binary file
 *
 * @param fileName String containing the name of the binary file
 */
void binary_on_screen(char *fileName);

#endif