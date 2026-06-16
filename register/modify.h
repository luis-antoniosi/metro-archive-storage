#ifndef REGISTER_MODIFY_H
#define REGISTER_MODIFY_H

#include "types.h"
#include "register.h"
#include "binFile/dataFile.h"

/**
 * @brief Reads a line from stdin and parses it into a register; space delimited and has quotes
 * 
 * @return Register* to the populated register or NULL if it fails
 */
Register *input_register();

/**
 * @brief Inserts a new register in a binary file
 * 
 * @param binFile Open binary file
 * @param data Register struct to be inserted
 * @param header File's header struct
 * 
 * @return SUCCESS or FAILURE
 */
Status insert_register(FILE *binFile, Register *data, DataHeader *header);

/**
 * @brief Removes a register by setting the removed flag and doing the appropriate changes
 *
 * @param binFile Open binary file
 * @param removedRRN RRN of the register
 */
void remove_register(FILE *binFile, int removedRRN);

/**
 * @brief Updates a binary file with a modified register based on search criteria (filters)
 * 
 * @param binFile Pointer to the open binary file
 * @param data Pointer to the register struct
 * @param filters Pointer to an array of SearchField
 * @param iterations Number of loops the function will do
 * 
 * @return SUCCESS or FAILURE
 */
Status update_register(FILE *binFile, Register *data, SearchField *filters, int iterations);


#endif