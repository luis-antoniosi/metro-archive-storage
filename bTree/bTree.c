#include <stdio.h>
#include <stdlib.h>
#include "bTree.h"
#include "binFile/indexFile.h" // for IndexHeader type

IndexPage *create_index_page()
{
    IndexPage *page = malloc(sizeof(IndexPage));

    if (!page)
        return NULL;

    page->removed = RECORD_ACTIVE;
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
    {
        if (fseek(indexFile, INDEX_HEADER_SIZE + (rrn * INDEX_PAGE_SIZE), SEEK_SET))
            return FAILURE;
    }

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

    if (fseek(indexFile, INDEX_HEADER_SIZE + (rrn * INDEX_PAGE_SIZE), SEEK_SET))
        return NULL;

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
        int mid = left + (right - left) / 2; // preventing overflow

        if (page->keys[mid].searchKey == searchKey)
            return mid;
        else if (page->keys[mid].searchKey < searchKey)
            left = mid + 1;
        else
            right = mid - 1;
    }

    // left is the index where the key should be inserted. it is returned as a negative number to show that it was not found.
    return -left - 1;
}

/**
 * @brief Recursively searches through the b-tree in indexFile to find the searchkey
 * 
 * @param indexFile Index file with all the indices
 * @param currentRRN RRN of the page that's currently being searched.
 * @param searchKey Key to be searched
 * @return Byte offset of the found searchKey. -1 if not found.
 */
static int search_recursive(FILE *indexFile, int currentRRN, int searchKey)
{
    // If there are no more subpages, hits a base case
    if (currentRRN == -1)
        return -1;

    IndexPage *page = read_index_page(indexFile, currentRRN);
    if (!page)
        return -1;

    // Checking if the searchkey is in the current page
    int result = binary_index_search(page, searchKey);

    // If it is, return its byteOffset
    if (result >= 0)
    {
        int offset = page->keys[result].byteOffset;
        free(page);
        return offset;
    }

    // If it was not found and the current page is a LEAF, we hit another base case
    if (page->nodeType == LEAF)
    {
        free(page);
        return -1;
    }

    // doing -result - 1 from binary_index_search gives us the subPage where the searchKey will be, if it exists 
    int childRRN = page->subPages[-result - 1];
    free(page);

    return search_recursive(indexFile, childRRN, searchKey);
}

int search_index_key(FILE *indexFile, IndexHeader *header, int searchKey)
{
    return search_recursive(indexFile, header->rootNode, searchKey);
}