#include <stdio.h>
#include <stdlib.h>
#include "bTree.h"

IndexHeader *create_index_header()
{
    IndexHeader *header = malloc(sizeof(IndexHeader));

    if (!header)
        return NULL;

    header->status = '0';
    header->rootNode = -1;
    header->top = -1;
    header->nextRRN = 0;
    header->numNodes = 0;

    return header;
}

Status write_index_header(FILE *indexFile, IndexHeader *header)
{
    if (!indexFile || !header)
        return FAILURE;

    if (fseek(indexFile, 0, SEEK_SET))
        return FAILURE;

    fwrite(&header->status, sizeof(char), 1, indexFile);
    fwrite(&header->rootNode, sizeof(int), 1, indexFile);
    fwrite(&header->top, sizeof(int), 1, indexFile);
    fwrite(&header->nextRRN, sizeof(int), 1, indexFile);
    fwrite(&header->numNodes, sizeof(int), 1, indexFile);

    return SUCCESS;
}

IndexHeader *read_index_header(FILE *indexFile)
{
    if (!indexFile)
        return NULL;

    if (fseek(indexFile, 0, SEEK_SET))
        return NULL;

    IndexHeader *header = create_index_header();

    if (fread(&header->status, sizeof(char), 1, indexFile) != 1 ||
        fread(&header->rootNode, sizeof(int), 1, indexFile) != 1 ||
        fread(&header->top, sizeof(int), 1, indexFile) != 1 ||
        fread(&header->nextRRN, sizeof(int), 1, indexFile) != 1 ||
        fread(&header->numNodes, sizeof(int), 1, indexFile) != 1)
    {
        printf("Unable to read BTree header.\n");
        free(header);
        return NULL;
    }

    return header;
}

//

IndexPage *create_index_page()
{
    IndexPage *page = malloc(sizeof(IndexPage));

    if (!page)
        return NULL;

    page->removed = '0';
    page->next = -1;
    page->nodeType = LEAF;
    page->keyCount = 1;

    for (int i = 0; i < TREE_ORDER - 1; i++)
        page->keys[i] = (IndexKey){-1, -1}; // placeholder key for initializing the page

    for (int i = 0; i < TREE_ORDER; i++)
        page->subPages[i] = -1;

    return page;
}

// if rrn is -1, it doesn't do the fseek (added to use in loops)
Status write_index_page(FILE *indexFile, IndexPage *page, int rrn)
{
    if (!indexFile || !page)
        return FAILURE;

    if (rrn != -1)
        fseek(indexFile, BT_HEADER_SIZE + (rrn * BT_PAGE_SIZE), SEEK_SET);

    fwrite(&page->removed, sizeof(char), 1, indexFile);
    fwrite(&page->next, sizeof(int), 1, indexFile);
    fwrite(&page->nodeType, sizeof(int), 1, indexFile);
    fwrite(&page->keyCount, sizeof(int), 1, indexFile);

    for (int i = 0; i < TREE_ORDER - 1; i++)
    {
        fwrite(&page->keys[i].searchKey, sizeof(int), 1, indexFile);
        fwrite(&page->keys[i].byteOffset, sizeof(int), 1, indexFile);
    }

    for (int i = 0; i < TREE_ORDER; i++)
    {
        fwrite(&page->subPages[i], sizeof(int), 1, indexFile);
    }

    return SUCCESS;
}

IndexPage *read_index_page(FILE *indexFile, int rrn)
{
    IndexPage *page = create_index_page();
    if (!indexFile || !page)
        return NULL;

    fseek(indexFile, BT_HEADER_SIZE + (rrn * BT_PAGE_SIZE), SEEK_SET);

    if (fread(&page->removed, sizeof(char), 1, indexFile) != 1 ||
        fread(&page->next, sizeof(int), 1, indexFile) != 1 ||
        fread(&page->nodeType, sizeof(int), 1, indexFile) != 1 ||
        fread(&page->keyCount, sizeof(int), 1, indexFile) != 1)
    {
        printf("Unable to read index page.\n");
        free(page);
        return NULL;
    }

    for (int i = 0; i < TREE_ORDER - 1; i++)
    {
        if (fread(&page->keys[i].searchKey, sizeof(int), 1, indexFile) != 1 ||
            fread(&page->keys[i].byteOffset, sizeof(int), 1, indexFile) != 1)
        {
            printf("Unable to read read key of index %d", i);
            free(page);
            return NULL;
        }
    }

    for (int i = 0; i < TREE_ORDER; i++)
    {
        if (fread(&page->subPages[i], sizeof(int), 1, indexFile) != 1)
        {
            printf("Unable to read subPages of index %d", i);
            free(page);
            return NULL;
        }
    }

    return page;
}

int binary_index_search(IndexPage *page, int searchKey)
{
    int left = 0, right = page->keyCount - 1;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        if (page->keys[mid].searchKey == searchKey)
            return mid;
        else if (page->keys[mid].searchKey < searchKey)
            left = mid + 1;
        else
            right = mid - 1;
    }

    return -left - 1;
}

static int search_recursive(FILE *indexFile, int currentRRN, int searchKey)
{
    if (currentRRN == -1)
        return -1;

    IndexPage *page = read_index_page(indexFile, currentRRN);
    if (!page)
        return -1;

    int result = binary_index_search(page, searchKey);

    if (result >= 0)
    {
        int offset = page->keys[result].byteOffset;
        free(page);
        return offset;
    }

    if (page->nodeType == LEAF)
    {
        free(page);
        return -1;
    }

    int childRRN = page->subPages[-result - 1];
    free(page);

    return search_recursive(indexFile, childRRN, searchKey);
}

int search_index_key(FILE *indexFile, IndexHeader *header, int searchKey)
{
    return search_recursive(indexFile, header->rootNode, searchKey);
}