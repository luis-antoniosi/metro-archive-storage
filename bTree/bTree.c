#include <stdio.h>
#include <stdlib.h>
#include "bTree.h"

BTHeader *create_btheader()
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

Status write_btheader(FILE *binFile, BTHeader *header)
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

BTHeader *read_btheader(FILE *binFile)
{
    if (!binFile)
        return NULL;

    if (fseek(binFile, 0, SEEK_SET))
        return NULL;

    BTHeader *header = create_btheader();

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
    BTPage *page = malloc(sizeof(BTPage));

    if (!page)
        return NULL;

    page->removed = '0';
    page->next = -1;
    page->nodeType = LEAF;
    page->keyCount = 1;

    for (int i = 0; i < TREE_ORDER - 1; i++)
        page->keys[i] = (BTKey){-1, -1}; // placeholder key for initializing the page

    for (int i = 0; i < TREE_ORDER; i++)
        page->subPages[i] = -1;

    return page;
}

// if rrn is -1, it doesn't do the fseek (added to use in loops)
Status write_page(FILE *binFile, BTPage *page, int rrn)
{
    if (!binFile || !page)
        return FAILURE;

    if (rrn != -1)
        fseek(binFile, HEADER_SIZE + (rrn * PAGE_SIZE), SEEK_SET);

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

BTPage *read_page(FILE *binFile, int rrn)
{
    BTPage *page = create_page();
    if (!binFile || !page)
        return NULL;

    fseek(binFile, HEADER_SIZE + (rrn * PAGE_SIZE), SEEK_SET);

    if (fread(&page->removed, sizeof(char), 1, binFile) != 1 ||
        fread(&page->next, sizeof(int), 1, binFile) != 1 ||
        fread(&page->nodeType, sizeof(int), 1, binFile) != 1 ||
        fread(&page->keyCount, sizeof(int), 1, binFile) != 1)
    {
        printf("Unable to read index page.\n");
        free(page);
        return NULL;
    }

    for (int i = 0; i < TREE_ORDER - 1; i++)
    {
        if (fread(&page->keys[i].searchKey, sizeof(int), 1, binFile) != 1 ||
            fread(&page->keys[i].byteOffset, sizeof(int), 1, binFile) != 1)
        {
            printf("Unable to read read key of index %d", i);
            free(page);
            return NULL;
        }
    }

    for (int i = 0; i < TREE_ORDER; i++)
    {
        if (fread(&page->subPages[i], sizeof(int), 1, binFile) != 1)
        {
            printf("Unable to read subPages of index %d", i);
            free(page);
            return NULL;
        }
    }

    return page;
}

static int binary_search(BTPage *page, int searchKey)
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

static int search_recursive(FILE *binFile, int currentRRN, int searchKey)
{
    if (currentRRN == -1)
        return -1;

    BTPage *page = read_page(binFile, currentRRN);
    if (!page)
        return -1;

    int result = binary_search(page, searchKey);

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

    return search_recursive(binFile, childRRN, searchKey);
}

int search_key(FILE *binFile, BTHeader *header, int searchKey)
{
    return search_recursive(binFile, header->rootNode, searchKey);
}

static void order_page(BTKey *workingKeys, int *workingPages, BTKey insertKey, int insertRRN, int count)
{
    int pos = count;
    while (pos > 0 && workingKeys[pos - 1].searchKey > insertKey.searchKey)
    {
        workingKeys[pos] = workingKeys[pos - 1];
        workingPages[pos + 1] = workingPages[pos];
        pos--;
    }

    workingKeys[pos] = insertKey;
    workingPages[pos + 1] = insertRRN;
}

static InsertResult insert_loop(FILE *binFile, BTHeader *header, int currentRRN, BTKey key, BTKey *promotedKey, int *rightChildRRN)
{
    BTPage *page = read_page(binFile, currentRRN);
    if (!page)
        return ERROR;

    int pos = 0;
    while (pos < page->keyCount && page->keys[pos].searchKey < key.searchKey)
        pos++;

    if (pos < page->keyCount && page->keys[pos].searchKey == key.searchKey)
    {
        free(page);
        return ERROR;
    }

    InsertResult result;
    if (page->nodeType == LEAF || page->subPages[0] == -1)
    {
        *promotedKey = key;
        *rightChildRRN = -1;
        result = PROMOTION;
    }
    else
        result = insert_loop(binFile, header, page->subPages[pos], key, promotedKey, rightChildRRN);

    if (result != PROMOTION)
    {
        free(page);
        return result;
    }

    if (page->keyCount < TREE_ORDER - 1)
    {
        order_page(page->keys, page->subPages, *promotedKey, *rightChildRRN, page->keyCount);
        page->keyCount++;

        write_page(binFile, page, currentRRN);
        free(page);

        return NO_PROMOTION;
    }
    else
    {
        BTPage *newPage = create_page();
        newPage->nodeType = page->nodeType;

        *promotedKey = split_page(page, newPage, *promotedKey, *rightChildRRN);
        *rightChildRRN = header->nextRRN;

        if (page->nodeType == ROOT)
        {
            page->nodeType = (page->subPages[0] == -1) ? LEAF : INTERMEDIARY;
            newPage->nodeType = page->nodeType;
        }

        write_page(binFile, page, currentRRN);
        write_page(binFile, newPage, header->nextRRN);

        header->nextRRN++;
        header->numNodes++;

        free(page);
        free(newPage);
        return PROMOTION;
    }
}

Status insert_key(FILE *binFile, BTHeader *header, BTKey key)
{
    if (!binFile || !header)
        return FAILURE;

    if (search_key(binFile, header, key.searchKey) != -1)
        return FAILURE;

    // if tree is empty, insert a new page as root (which for some reason needs to be -1, since it's also a LEAF)
    if (header->rootNode == -1)
    {
        BTPage *root = create_page();
        root->nodeType = LEAF; // see comment above
        root->keys[0] = key;
        root->keyCount = 1;

        header->rootNode = header->nextRRN;

        write_page(binFile, root, header->rootNode);
        // still need to actually update this header
        header->nextRRN++;
        header->numNodes++;

        free(root);
        return SUCCESS;
    }

    // otherwise
    BTKey promotedKey;
    int rightChildRRN = -1;

    InsertResult result = insert_loop(binFile, header, header->rootNode, key, &promotedKey, &rightChildRRN);

    if (result == ERROR)
        return FAILURE;

    if (result == PROMOTION)
    {
        BTPage *newRoot = create_page();
        newRoot->nodeType = ROOT;
        newRoot->keys[0] = promotedKey;
        newRoot->keyCount = 1;
        newRoot->subPages[0] = header->rootNode;
        newRoot->subPages[1] = rightChildRRN;

        header->rootNode = header->nextRRN;
        write_page(binFile, newRoot, header->rootNode);

        header->nextRRN++;
        header->numNodes++;

        free(newRoot);
    }

    return SUCCESS;
}

BTKey split_page(BTPage *page, BTPage *newPage, BTKey insertKey, int insertRRN)
{
    BTKey workingKeys[TREE_ORDER];
    int workingPages[TREE_ORDER + 1];

    for (int i = 0; i < TREE_ORDER - 1; i++)
        workingKeys[i] = page->keys[i];
    for (int i = 0; i < TREE_ORDER; i++)
        workingPages[i] = page->subPages[i];

    order_page(workingKeys, workingPages, insertKey, insertRRN, TREE_ORDER - 1);

    int promoIdx = TREE_ORDER / 2;
    BTKey promoKey = workingKeys[promoIdx];

    for (int i = 0; i < TREE_ORDER - 1; i++)
    {
        if (i < promoIdx)
            page->keys[i] = workingKeys[i];
        else
            page->keys[i] = (BTKey){-1, -1};
    }

    page->keyCount = promoIdx;

    for (int i = 0; i <= promoIdx; i++)
        page->subPages[i] = workingPages[i];

    int rightCount = 0;
    for (int i = promoIdx + 1; i < TREE_ORDER; i++)
    {
        page->subPages[i] = -1;
        newPage->keys[rightCount++] = workingKeys[i];
    }

    newPage->keyCount = rightCount;

    for (int i = 0; i <= rightCount; i++)
        newPage->subPages[i] = workingPages[promoIdx + i + 1];

    newPage->nodeType = page->nodeType;

    return promoKey;
}