#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "types.h"

#define END_FILE (FILE *)-1
#define CLOSE_FILES(...) close_files(__VA_ARGS__, END_FILE)
#define CLOSE_FILES_FAILURE(...)            \
    {                                       \
        close_files(__VA_ARGS__, END_FILE); \
        print_file_failure();               \
    }

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
 * @brief 
 * 
 * @param binFile 
 * @return Status 
 */
Status check_header_consistency(FILE *binFile);

/**
 * @brief 
 * 
 * @param firstFile 
 * @param ... 
 */
void close_files(FILE *firstFile, ...);

#endif