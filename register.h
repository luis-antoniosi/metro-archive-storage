#ifndef REGISTER_H
#define REGISTER_H

#include <stdio.h>
#include "types.h"

/**
 * @brief Parses a delimited string buffer and populates a register
 *
 * This function takes a line of text and splits it by commas.
 *
 * @param buffer Pointer to a string that represents a single record.
 *
 * @return Register* A pointer to the allocated Register or NULL if the allocation fails
 */
Register *parse_register(char *buffer);

/**
 * @brief Writes a Register struct into a binary format
 *
 * @param binFile A pointer to the open binary file
 * @param data A pointer to the struct to be written
 */
void write_register(FILE *binFile, Register *data);

/**
 * @brief reads a single record from a binary file into a register struct
 *
 * @param binFile A pointer to the open binary file
 *
 * @return Register* Pointer to the dinamically allocated register or NULL if
 *  the end of the file is reached, a read error ocurr or the allocation fails
 */
Register *read_register(FILE *binFile);

/**
 * @brief Prints a single Register
 *
 * @param data Pointer to the register
 */
void print_register(Register *Register);

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

/**
 * @brief removes a register by setting the removed flag and pushing on the RRN stack
 *
 * @param binFile Open binary file
 */
void remove_register(FILE *binFile);

/**
 * @brief Recalculates the numStations and numPairStations count
 * 
 * @param binFile Pointer to the open binary file
 * @param header Pointer to the Header struct that will be updated
 * 
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus update_station_counts(FILE *binFile, Header *header);

/**
 * @brief reads a line from stdin and parses it into a register
 * 
 * @return Register* Pointer to the populated register or NULL if fail
 */
Register *input_register();

/**
 * @brief inserts a new register in a binary file
 * 
 * @param binFile Open binary file
 * @param data Register struct to be inserted
 * @param header File`s header struct
 * 
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus insert_register(FILE *binFile, Register *data, Header *header);

/**
 * @brief updates a binary file with a modified register
 * 
 * @param binFile Pointer to the open binary file
 * @param data Pointer to the register struct
 * 
 * @return DATA_SUCESS or DATA_FAILURE
 */
DataStatus update_register(FILE *binFile, Register *data);

/**
 * @brief free the memory of a register
 *
 * @param data double pointer to the register
 */
void destroy_register(Register **Register);

#endif