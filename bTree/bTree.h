#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include "types.h"

#define TREE_ORDER 4
#define HEADER_SIZE 17
#define PAGE_SIZE 53

typedef struct BTHeader {
    char status;
    int rootNode;   // -1 if the tree is empty
    int top;        // -1 if there were no logically removed registers
    int nextRRN;    //  0 if the tree is empty
    int numNodes;   //  0 if the tree is empty
} BTHeader;

// both fields should be -1 if the page hasn't been filled
typedef struct BTKey
{
    int searchKey;
    int byteOffset;
} BTKey;

typedef struct BTPage
{
    char removed;
    int next;

    int nodeType; // -1 is for leaves, 0 for root and 1 for intermediary nodes.
    int keyCount;

    BTKey keys[TREE_ORDER - 1];
    int subPages[TREE_ORDER];   // -1 for null "pointers"
} BTPage;

typedef enum NodeType {
    LEAF = -1,
    ROOT = 0,
    INTERMEDIARY = 1
} NodeType;

typedef enum InsertResult {
    PROMOTION,
    NO_PROMOTION,
    ERROR
} InsertResult;

BTHeader *create_btheader();
Status write_btheader(FILE *binFile, BTHeader *header);
BTHeader *read_btheader(FILE *binFile);

BTPage *create_page();
Status write_page(FILE *binFile, BTPage *page, int rrn);
BTPage *read_page(FILE *binFile, int rrn);

Status insert_key(FILE *binFile, BTHeader *header, BTKey key);
BTKey split_page(BTPage *page, BTPage *newPage, BTKey insertKey, int insertRRN);

#endif