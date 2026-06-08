#ifndef SEARCH_H
#define SEARCH_H

#include <stdio.h>
#include "register.h"

/**
 * @brief reads a Register from a file and evaluates if it meets all the serch filters
 *
 * The function extracts the next register from the binFile and compares it to a array of filters
 * applying the 'AND' logic.
 *
 * @param binFile Pointer to the binary file
 * @param filters array containing the search filters
 * @param pairInterations Number of filters in the array
 *
 * @return Register* Pointer to the read register or NULL at EOF.
 */
Register *check_register_field_search(FILE *binFile, SearchField *filters, int pairIterations);


/**
 * @brief Reads the search filters typed by the user
 *
 * @param pairInterations Pointer to the int variable that the number of filters is assigned to
 *
 * @return SearchField* Allocated array containing the filters in a struct or NULL in case of failure,
 *  the caller must free the dinamically alocated array.
 */
SearchField *get_all_search_fields(int *pairIterations);

#endif