#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include "types.h"

//-----------------------------------------------//
//                    CONSTANTS                  //
//----------------------------------------------//

#define TREE_ORDER          4
#define BT_HEADER_SIZE      17
#define BT_PAGE_SIZE        53

#define MIN_OCCUPANCY       (((TREE_ORDER + 2 - 1) / 2) - 1)

//------------------------------------------------//
//                    STRUCTURES                  //
//------------------------------------------------//

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

//-----------------------------------------------//
//                    FUNCTIONS                  //
//-----------------------------------------------//

// TODO: Comments for these functions
IndexHeader *create_index_header();
Status write_index_header(FILE *indexFile, IndexHeader *header);
IndexHeader *read_index_header(FILE *indexFile);

IndexPage *create_index_page();
Status write_index_page(FILE *indexFile, IndexPage *page, int rrn);
IndexPage *read_index_page(FILE *indexFile, int rrn);

int binary_index_search(IndexPage *page, int searchkey);
int search_index_key(FILE *indexFile, IndexHeader *header, int searchKey);

#endif