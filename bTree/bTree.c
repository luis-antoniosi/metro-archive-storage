#include <stdio.h>
#include <stdlib.h>
#include "bTree.h"

BTHeader *create_header()
{
    BTHeader *header = malloc(sizeof(BTHeader));

    if (!header)
        return NULL;

    header->status = '0';
    header->rootNode = -1;
    header->top = -1;
    header->nextRRN = 0;
    header->numNodes = 0;

    return header;
}

Status write_header(FILE *binFile, BTHeader *header)
{
    if (!binFile || !header)
        return FAILURE;

    if (fseek(binFile, 0, SEEK_SET))
        return FAILURE;

    fwrite(&header->status, sizeof(char), 1, binFile);
    fwrite(&header->rootNode, sizeof(int), 1, binFile);
    fwrite(&header->top, sizeof(int), 1, binFile);
    fwrite(&header->nextRRN, sizeof(int), 1, binFile);
    fwrite(&header->numNodes, sizeof(int), 1, binFile);

    return SUCCESS;
}

BTHeader *read_header(FILE *binFile)
{
    if (!binFile)
        return NULL;

    if (fseek(binFile, 0, SEEK_SET))
        return NULL;

    BTHeader *header = create_header();

    if (fread(&header->status, sizeof(char), 1, binFile) != 1 ||
        fread(&header->rootNode, sizeof(int), 1, binFile) != 1 ||
        fread(&header->top, sizeof(int), 1, binFile) != 1 ||
        fread(&header->nextRRN, sizeof(int), 1, binFile) != 1 ||
        fread(&header->numNodes, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read BTree header.\n");
        free(header);
        return NULL;
    }

    return header;
}

//

BTPage *create_page()
{
    BTPage *page = malloc(NODE_SIZE);

    page->removed = '0';
    page->next = -1;
    page->nodeType = LEAF;
    page->keyCount = 1;

    BTKey emptyKey = {-1, -1};
    for (int i = 0; i < TREE_ORDER - 1; i++)
        page->keys[i] = emptyKey;

    for (int i = 0; i < TREE_ORDER; i++)
        page->subPages[i] = -1;

    return page;
}

Status write_page(FILE *binFile, BTPage *page)
{
    if (!binFile || !page)
        return FAILURE;

    fwrite(&page->removed, sizeof(char), 1, binFile);
    fwrite(&page->next, sizeof(int), 1, binFile);
    fwrite(&page->nodeType, sizeof(int), 1, binFile);
    fwrite(&page->keyCount, sizeof(int), 1, binFile);

    for (int i = 0; i < TREE_ORDER - 1; i++)
    {
        fwrite(&page->keys[i].searchKey, sizeof(int), 1, binFile);
        fwrite(&page->keys[i].byteOffset, sizeof(int), 1, binFile);
    }

    for (int i = 0; i < TREE_ORDER; i++)
    {
        fwrite(&page->subPages[i], sizeof(int), 1, binFile);
    }

    return SUCCESS;
}

// todo: add "fread" error handling
BTPage *read_page(FILE *binFile, int RRN)
{
    BTPage *page = create_page();
    if (!binFile || !page)
        return NULL;

    fseek(binFile, HEADER_SIZE + (RRN * NODE_SIZE), SEEK_SET);

    fread(&page->removed, sizeof(char), 1, binFile);
    fread(&page->next, sizeof(int), 1, binFile);
    fread(&page->nodeType, sizeof(int), 1, binFile);
    fread(&page->keyCount, sizeof(int), 1, binFile);

    for (int i = 0; i < TREE_ORDER - 1; i++)
    {
        fread(&page->keys[i].searchKey, sizeof(int), 1, binFile);
        fread(&page->keys[i].byteOffset, sizeof(int), 1, binFile);
    }

    for (int i = 0; i < TREE_ORDER; i++)
        fread(&page->subPages[i], sizeof(int), 1, binFile);

    return page;
}