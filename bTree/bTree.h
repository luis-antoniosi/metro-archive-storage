#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include "types.h"

#define TREE_ORDER 4
#define NODE_SIZE 53

typedef struct BTHeader {
    char status;
    int rootNode;   // -1 if the tree is empty
    int top;        // -1 if there were no logically removed registers
    int nextRRN;    //  0 if the tree is empty
    int numNodes;   //  0 if the tree is empty
} BTHeader;

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
    int subPages[TREE_ORDER];
} BTPage;

BTHeader *create_header();
Status write_header(FILE *binFile, BTHeader *header);
BTHeader *read_header(FILE *binFile);

BTPage *create_page();
Status write_page(FILE *binFile, BTPage *page);
BTPage *read_page();

#endif