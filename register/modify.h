#ifndef MODIFY_H
#define MODIFY_H

#include "register.h"
#include "../types.h"

/**
 * @brief reads a line from stdin and parses it into a register; space delimited and has quotes
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
 * @return SUCCESS or FAILURE
 */
Status insert_register(FILE *binFile, Register *data, Header *header);

/**
 * @brief removes a register by setting the removed flag and pushing on the RRN stack
 *
 * @param binFile Open binary file
 */
void remove_register(FILE *binFile);

/**
 * @brief updates a binary file with a modified register
 * 
 * @param binFile Pointer to the open binary file
 * @param data Pointer to the register struct
 * 
 * @return SUCCESS or FAILURE
 */
Status update_register(FILE *binFile, Register *data, SearchField *filters, int iterations);


#endif