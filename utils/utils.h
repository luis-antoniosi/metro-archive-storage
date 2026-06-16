#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

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

#endif