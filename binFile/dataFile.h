#ifndef DATA_FILE_H
#define DATA_FILE_H

#include <stdio.h>
#include "types.h"

//---------------------------------------//
//             FILE HEADER               //
//---------------------------------------//

#define HEADER_SIZE 17

/**
 * @struct DataHeader
 * @brief Represents the header record of the binary data file.
 */
typedef struct DataHeader
{
    char status;            // STATUS_CONSISTENT or STATUS_INCONSISTENT; see types.h
    int top;                // byte offset of the last removed register, -1 if there are none
    int nextRRN;            // initially 0
    int numStations;        // initially 0
    int numPairStations;    // initially 0
} DataHeader;

/**
 * @brief Creates a DataHeader struct and sets it with default values
 *
 * @return DataHeader* pointer to the dynamically allocated DataHeader
 */
DataHeader *create_data_header();

/**
 * @brief Writes DataHeader to a file
 *
 * @param regFile File that the header will be written to
 * @param header DataHeader to be written
 *
 * @return SUCCESS or FAILURE
 */
Status write_data_header(FILE *regFile, DataHeader *header);

/**
 * @brief Reads a DataHeader from a file
 * 
 * @param regFile File that contains the header
 * 
 * @return DataHeader* pointing to the the read header
 */
DataHeader *read_data_header(FILE *regFile);

/**
 * @brief Loads, updates and writes the file's header with updated numStations and numPairStations
 * 
 * @param regFile Open binary file
 * 
 * @return SUCCESS or FAILURE
 */
Status update_data_header_count(FILE *regFile);

//------------------------------------------//
//             FILE FUNCTIONS               //
//------------------------------------------//

// Write file
/**
 * @brief Writes a binary file with registers from a input .csv file
 *
 * @param inputPath Path to the csv file
 * @param outputPath Binary file's path which will be written to
 *
 * @return SUCCESS or FAILURE
 */
Status write_data_file(char *inputPath, char *outputPath);

// Print
/**
 * @brief Prints all registers from a binary file
 *
 * @param dataPath Path to the binary file
 *
 * @return SUCCESS or FAILURE
 */
Status print_all_data(char *dataPath);

/**
 * @brief Prints all registers that meet the filters' values
 *
 * @param dataPath Path to the binary file
 * @param iterations Number of searches
 *
 * @return SUCCESS or FAILURE
 */
Status print_all_data_where(char *dataPath, int iterations);

// Delete
/**
 * @brief Deletes all registers that meet the filters' vlaues
 * 
 * @param dataPath Path to the binary file
 * @param iterations Number of filter iterations
 * 
 * @return SUCCESS or FAILURE
 */
Status delete_all_data_where(char *dataPath, int iterations);

/**
 * @brief Inserts multiple registers in a binary file
 * 
 * @param dataPath Path to the binary file
 * @param iterations Number of insertions
 * 
 * @return SUCCESS or FAILURE
 */
Status insert_data(char *dataPath, int iterations);

/**
 * @brief Searches for registers that meet the filters' values, updates them
 * 
 * @param dataPath Path to the binary file
 * @param iterations Number of filter iterations
 * 
 * @return SUCCESS or FAILURE
 */
Status update_data_where(char *dataPath, int iterations);

Status select_join(char *sourcePath, char *joinPath);

Status order_by(char *dataPath, char *field, char *orderedPath);

Status select_join_order_by(char *sourcePath, char *joinPath);

#endif