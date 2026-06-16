#ifndef INDEX_FILE_H
#define INDEX_FILE_H

#include <stdio.h>
#include "types.h"

//---------------------------------------//
//             FILE HEADER               //
//---------------------------------------//

/**
 * @struct IndexHeader
 * @brief Represents the header of an index file, which uses a b-tree.
 */
typedef struct IndexHeader {
    char status;
    int rootNode;   // -1 if the tree is empty
    int top;        // -1 if there were no logically removed registers
    int nextRRN;    //  0 if the tree is empty
    int numNodes;   //  0 if the tree is empty
} IndexHeader;

//------------------------------------------//
//             FILE FUNCTIONS               //
//------------------------------------------//


// IndexHeader functions

/**
 * @brief Create an IndexHeader, returns it with default values.
 * 
 * @return Pointer to the created IndexHeader
 */
IndexHeader *create_index_header();

/**
 * @brief Writes the header in the indexFile
 * 
 * @param indexFile File where the header will be written into
 * @param header Header that'll be written
 * @return SUCCESS or FAILURE
 */
Status write_index_header(FILE *indexFile, IndexHeader *header);

/**
 * @brief Returns the current index header of an index file
 * 
 * @param indexFile File with the header 
 * @return Pointer to the read IndexHeader
 */
IndexHeader *read_index_header(FILE *indexFile);

//

/**
 * @brief Creates an index in indexFile for each non-removed register in registerFile.
 * 
 * @param registerFile File with all the registers
 * @param indexFile File where the index will be created
 * @return SUCCESS or FAILURE
 */
Status create_index(FILE *registerFile, FILE *indexFile);

/**
 * @brief Searches and prints a station based on filters. Uses the index file only when the filter has a "stationCode" field.
 * 
 * @param registerFile File with all the registers
 * @param indexFile File where the indices are stored
 * @param iterations Number of search iterations
 * @return SUCCESS or FAILURE 
 */
Status search_with_index(FILE *registerFile, FILE *indexFile, int iterations);

/**
 * @brief Inserts a register in registerFile and its respective index in indexFile, as long as it doesn't already exist. 
 * 
 * @param registerFile File where a new register will be inserted in.
 * @param indexFile File where all indices are stored, and where the new index will be inserted.
 * @param iterations Number of insert iterations
 * @return SUCCESS or FAILURE 
 */
Status insert_index(FILE *registerFile, FILE *indexFile, int iterations);

/**
 * @brief Deletes a register in registerFile and its respective index in indexFile, as long as it's not already logically removed.
 * 
 * @param registerFile File where a register will be deleted from
 * @param indexFile File where an index will be deleted from
 * @param iterations Number of delete iterations
 * @return SUCCESS or FAILURE
 */
Status delete_index(FILE *registerFile, FILE *indexFile, int iterations);

#endif