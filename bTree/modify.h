#ifndef BTREE_MODIFY_H
#define BTREE_MODIFY_H

#include <stdio.h>
#include "types.h"
#include "bTree.h"

/**
 * @enum InsertResult
 * @brief Different cases the insert_loop function can return
 */
typedef enum InsertResult {
    PROMOTION,
    NO_PROMOTION,
    ERROR
} InsertResult;

/**
 * @enum RemoveResult
 * @brief Different cases the remove_loop and handle_underflow functions can return
 */
typedef enum RemoveResult {
    REMOVED,
    NOT_FOUND,
    REMOVE_ERROR,
    REMOVED_UNDERFLOW
} RemoveResult;

/**
 * @brief Inserts a key into the indexFile's b-tree.
 * 
 * @param indexFile File containing all indices, open in "rb+"
 * @param header indexFile's header
 * @param key Key to insert
 * @return SUCCESS or FAILURE 
 */
Status insert_index_key(FILE *indexFile, IndexHeader *header, IndexKey key);

/**
 * @brief Removes a key from the indexFile's b-tree.
 * 
 * @param indexFile File containing all indices, open in "rb+"
 * @param header indexFile's header
 * @param removedKey Identifier of the key to be removed
 * @return SUCCESS or FAILURE
 */
Status remove_index_key(FILE *indexFile, IndexHeader *header, int removedKey);

#endif