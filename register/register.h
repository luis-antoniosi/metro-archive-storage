#ifndef REGISTER_H
#define REGISTER_H

#include <stdio.h>
#include "../types.h"

/**
 * @brief Parses a delimited string buffer (csv) and populates a register
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
void print_register(Register *data);

/**
 * @brief free the memory of a register
 *
 * @param data double pointer to the register
 */
void destroy_register(Register **Register);

#endif