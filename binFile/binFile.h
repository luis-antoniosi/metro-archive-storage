#ifndef BINFILE_H
#define BINFILE_H

#include <stdio.h>
#include "types.h"
#include "register/register.h"

// Write file
/**
 * @brief writes a binary file with registers from a input .csv file
 *
 * @param inputFile Open input .csv file in "r" mode
 * @param outputFile Open output binary file in "wb+" mode
 *
 * @return DATA_SUCCESS if sucesseful or DATA_FAILURE if unsucesseful
 */
Status write_bin_file(FILE *inputFile, FILE *outputFile);

// Print
/**
 * @brief prints all registers from a binary file
 *
 * @param binFile Open binary file
 *
 * @return DATA_SUCESS or DATA_FAILURE
 */
Status print_all_data(FILE *binFile);

/**
 * @brief prints all registers that meets the filters requirements
 *
 * @param binFile Open binary file
 * @param iterations Number of searches
 *
 * @return DATA_SUCESS or DATA_FAILURE
 */
Status print_all_data_where(FILE *binFile, int iterations);

// Delete
/**
 * @brief deletes all registers that meets the filters requirements
 */
Status delete_all_data_where(FILE *binFile, int iterations);

/**
 * @brief Inserts multiple registers in a binary file
 * 
 * @param binFile Pointer to the open binary file
 * @param iterations number of insertions
 * 
 * @return DATA_SUCESS or DATA_FAILURE
 */
Status insert_data(FILE *binFile, int iterations);

/**
 * @brief Searchs for records that matches a criteria and updates them
 * 
 * @param binFile Open binary file
 * @param iterations number of update operations
 * 
 * @return DATA_SUCESS or DATA_FAILURE
 */
Status update_data_where(FILE *binFile, int iterations);

// part 2

Status create_index(FILE *registerFile, FILE *indexFile);
Status insert_index(FILE *registerFile, FILE *indexFile, int iterations);

//

// Print binary
/**
 * @brief Prints a checksum to validate a binary file
 *
 * @param fileName String containing the name of the binary file
 */
void binary_on_screen(char *fileName);

#endif