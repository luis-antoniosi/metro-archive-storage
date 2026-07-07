#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "types.h"

#define END_FILE (FILE *)-1
#define CLOSE_FILES(...) close_files(__VA_ARGS__, END_FILE)

// Print binary
/**
 * @brief Prints a checksum to validate a binary file
 *
 * We didn't really make our own, just copied and changed the variables' names
 * 
 * @param fileName String containing the name of a binary file
 */
void binary_on_screen(char *fileName);

/**
 * @brief Writes a specified status to the beginning of a file
 *
 * @param binFile File where the status will be written
 * @param status Char for the status; STATUS_CONSISTENT ('1') or STATUS_INCONSISTENT ('0')
 */
void change_status(FILE *binFile, char status);

/**
 * @brief Checks if the header of a file is consistents (STATUS_CONSISENT or STATUS_INCONSISTENT)
 * 
 * @param binFile File that'll be checked
 * @return SUCCESS if it is consistent, FAILURE if not.
 */
Status check_header_consistency(FILE *binFile);

/**
 * @brief Variadic function that closes a variable number of files, NULL or not.
 * It should be called using CLOSE_FILES, because the END_FILE pointer is needed for a check.
 * 
 * @param firstFile First file of the list
 * @param ... All files separated by commas
 */
void close_files(FILE *firstFile, ...);

#endif