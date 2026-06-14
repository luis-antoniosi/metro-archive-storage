#ifndef SEARCH_H
#define SEARCH_H

#include <stdio.h>
#include "register.h"

/**
 * @brief reads a Register from a file and evaluates if it meets all the search filters
 *
 * This function extracts the next register from a binFile and compares it to an array of filters.
 *
 * @param binFile Pointer to the binary file
 * @param filters Array containing the search field filters
 * @param pairIterations Number of filters in the array
 *
 * @return Register* of a register that meets all search filters, NULL otherwise
 */
Register *check_register_field_search(FILE *binFile, SearchField *filters, int pairIterations);

/**
 * @brief Reads the filters typed by the user
 *
 * @param pairIterations Pointer to an int variable the number of filters is assigned to
 *
 * @return SearchField* Allocated array containing the filters in a struct or NULL in case of failure.
 *  The caller must free the dynamically allocated array.
 */
SearchField *get_all_search_fields(int *pairIterations);

#endif