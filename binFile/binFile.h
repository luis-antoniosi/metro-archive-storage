#ifndef BINFILE_H
#define BINFILE_H

#include <stdio.h>
#include "types.h"

// Write file
/**
 * @brief Writes a binary file with registers from a input .csv file
 *
 * @param inputFile Open input .csv file in "r" mode
 * @param outputFile Open output binary file in "wb+" mode
 *
 * @return SUCCESS or FAILURE
 */
Status write_bin_file(FILE *inputFile, FILE *outputFile);

// Print
/**
 * @brief Prints all registers from a binary file
 *
 * @param binFile Open binary file
 *
 * @return SUCCESS or FAILURE
 */
Status print_all_data(FILE *binFile);

/**
 * @brief Prints all registers that meet the filters' values
 *
 * @param binFile Open binary file
 * @param iterations Number of searches
 *
 * @return SUCCESS or FAILURE
 */
Status print_all_data_where(FILE *binFile, int iterations);

// Delete
/**
 * @brief Deletes all registers that meet the filters' vlaues
 * 
 * @param binFile Open binary file
 * @param iterations Number of filter iterations
 * 
 * @return SUCCESS or FAILURE
 */
Status delete_all_data_where(FILE *binFile, int iterations);

/**
 * @brief Inserts multiple registers in a binary file
 * 
 * @param binFile Pointer to the open binary file
 * @param iterations Number of insertions
 * 
 * @return SUCCESS or FAILURE
 */
Status insert_data(FILE *binFile, int iterations);

/**
 * @brief Searches for registers that meet the filters' values, updates them
 * 
 * @param binFile Open binary file
 * @param iterations Number of filter iterations
 * 
 * @return SUCCESS or FAILURE
 */
Status update_data_where(FILE *binFile, int iterations);

// part 2
// TODO: Add comments for these functions; maybe make them be in another file indexFile.c/.h
Status create_index(FILE *registerFile, FILE *indexFile);
Status search_with_index(FILE *registerFile, FILE *indexFile, int iterations);
Status insert_index(FILE *registerFile, FILE *indexFile, int iterations);
Status delete_index(FILE *registerFile, FILE *indexFile, int iterations);

//

// Print binary
/**
 * @brief Prints a checksum to validate a binary file
 *
 * @param fileName String containing the name of a binary file
 */
void binary_on_screen(char *fileName);

#endif