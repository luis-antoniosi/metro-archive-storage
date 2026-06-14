#ifndef REGISTER_H
#define REGISTER_H

#include <stdio.h>
#include "types.h" // for BUF_SIZE

/**
 * @struct Register
 * @brief Represents a station data record, mapping its logical fields.
 */
typedef struct Register
{
    char removed;   // '0' for not removed, '1' for removed
    int next;       // rrn of the next logically removed register; should be -1 when necessary

    int stationCode;    
    int lineCode;

    int nextStationCode;
    int distNextStation;

    int codeIntegLine;
    int codeIntegStation;

    int sizeStationName;
    char *stationName;

    int sizeLineName;
    char *lineName;
} Register;

// ----------------------- //
//   AUXILIARY STRUCTURES  //
// ----------------------- //

/**
 * @struct SearchField
 * @brief Stores the pair (name, value) for search, update, or deletion operations.
 */
typedef struct SearchField
{
    char name[BUF_SIZE];    // field name
    char value[BUF_SIZE];   // searched value
} SearchField;

/**
 * @struct StationPair
 * @brief Auxiliary structure to count unique pairs of stations.
 */
typedef struct StationPair
{
    int stationCode;
    int nextStationCode;
} StationPair;

//-----------------------------------------------//
//                    FUNCTIONS                  //
//-----------------------------------------------//

/**
 * @brief Parses a delimited string buffer (csv) and populates a register
 *
 * This function takes a line of text and splits it by commas.
 *
 * @param buffer Pointer to a string that represents a single record.
 *
 * @return Register* to the allocated Register or NULL if the allocation fails
 */
Register *parse_register(char *buffer);

/**
 * @brief Writes a Register struct into a binary file
 *
 * @param binFile A pointer to the open binary file
 * @param data A pointer to the struct to be written
 */
void write_register(FILE *binFile, Register *data);

/**
 * @brief Reads a single record from a binary file, turns it into a register struct
 *
 * @param binFile A pointer to the open binary file
 *
 * @return Register* to the dynamically allocated register or NULL if
 *  the end of the file is reached, a read error occurs or the allocation fails
 */
Register *read_register(FILE *binFile);

/**
 * @brief Prints a single Register
 *
 * @param data Pointer to the register
 */
void print_register(Register *data);

/**
 * @brief Frees the memory of a register
 *
 * @param data Double pointer to the register
 */
void destroy_register(Register **Register);

#endif