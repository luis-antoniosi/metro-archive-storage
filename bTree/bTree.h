#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include "types.h"
#include "binFile/indexFile.h"  // for IndexHeader type

//-----------------------------------------------//
//                    CONSTANTS                  //
//----------------------------------------------//

#define TREE_ORDER              4
#define INDEX_HEADER_SIZE      17
#define INDEX_PAGE_SIZE        53

#define MIN_OCCUPANCY           (((TREE_ORDER + 2 - 1) / 2) - 1)

//------------------------------------------------//
//                    STRUCTURES                  //
//------------------------------------------------//

// Header is in binFile/indexFile.h

// both fields should be -1 if the page hasn't been filled
/**
 * @struct IndexKey 
 * @brief Represents the keys a b-tree's page stores. Both fields should be -1 if the page hasn't been filled. 
 */
typedef struct IndexKey
{
    int searchKey;
    int byteOffset;
} IndexKey;

/**
 * @struct IndexPage
 * @brief Represents the page of a b-tree.
 */
typedef struct IndexPage
{
    char removed;
    int next;

    int nodeType; // -1 is for leaves, 0 for root and 1 for intermediary nodes; see the NodeType enum below
    int keyCount;

    IndexKey keys[TREE_ORDER - 1];
    int subPages[TREE_ORDER];   // -1 for null "pointers"
} IndexPage;

//---------------------------------//
//             ENUMS               //
//---------------------------------//

/**
 * @enum Status
 * @brief Indicates the node type of a IndexPage
 */
typedef enum NodeType {
    LEAF = -1,
    ROOT = 0,
    INTERMEDIARY = 1
} NodeType;

//-----------------------------------------------//
//                    FUNCTIONS                  //
//-----------------------------------------------//


// IndexPage functions

/**
 * @brief Creates an IndexPage, returns it with default values.
 * 
 * @return Pointer to the created IndexPage
 */
IndexPage *create_index_page();

/**
 * @brief Writes an IndexPage in the indexFile, using the RRN as its position. If the RRN is -1, just writes it in the current position.
 * 
 * @param indexFile File where the index will be written into
 * @param page IndexPage that'll be written
 * @param rrn  Position of where the page will be written
 * @return SUCCESS or FAILURE 
 */
Status write_index_page(FILE *indexFile, IndexPage *page, int rrn);

/**
 * @brief Reads an IndexPage at the specified RRN.
 * 
 * @param indexFile File containing all indices
 * @param rrn Position of the page that'll be read
 * @return Pointer to the read IndexPage
 */
IndexPage *read_index_page(FILE *indexFile, int rrn);

// Search functions

/**
 * @brief Uses a binary_search algorithm to return the array index of a searchKey in a page.
 * 
 * @param page Page that'll be searched
 * @param searchkey Value to search
 * @return Index of the key in the keys array of the page. 
 * Negative number if not found, which also means it's the subPage's index of where the key "would be", if it existed (or if it simply was not in the current page).
 */
int binary_index_search(IndexPage *page, int searchkey);

/**
 * @brief Searches for a searchKey in the indexFile
 * 
 * @param indexFile File with all the indices
 * @param header Header of the indexFile
 * @param searchKey Value to search
 * @return Byte offset of the searchedKey. -1 if not found.
 */
int search_index_key(FILE *indexFile, IndexHeader *header, int searchKey);

#endif