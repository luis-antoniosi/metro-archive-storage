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
 * @struct BTHeader
 * @brief Represents the header of an index file, which uses a b-tree.
 */
typedef struct BTHeader {
    char status;
    int rootNode;   // -1 if the tree is empty
    int top;        // -1 if there were no logically removed registers
    int nextRRN;    //  0 if the tree is empty
    int numNodes;   //  0 if the tree is empty
} BTHeader;

// both fields should be -1 if the page hasn't been filled
/**
 * @struct Represents the keys a b-tree's page stores.
 * @brief Both fields should be -1 if the page hasn't been filled. 
 */
typedef struct BTKey
{
    int searchKey;
    int byteOffset;
} BTKey;

/**
 * @struct BTPage
 * @brief Represents the page of a b-tree.
 */
typedef struct BTPage
{
    char removed;
    int next;

    int nodeType; // -1 is for leaves, 0 for root and 1 for intermediary nodes; see the NodeType enum below
    int keyCount;

    BTKey keys[TREE_ORDER - 1];
    int subPages[TREE_ORDER];   // -1 for null "pointers"
} BTPage;

//---------------------------------//
//             ENUMS               //
//---------------------------------//

/**
 * @enum Status
 * @brief Indicates the node type of a BTPage
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
BTHeader *create_btheader();
Status write_btheader(FILE *binFile, BTHeader *header);
BTHeader *read_btheader(FILE *binFile);

BTPage *create_page();
Status write_page(FILE *binFile, BTPage *page, int rrn);
BTPage *read_page(FILE *binFile, int rrn);

int search_key(FILE *binFile, BTHeader *header, int searchKey);

Status insert_key(FILE *binFile, BTHeader *header, BTKey key);
BTKey split_page(BTPage *page, BTPage *newPage, BTKey insertKey, int insertRRN);

Status remove_key(FILE *binFile, BTHeader *header, int removedKey);

#endif