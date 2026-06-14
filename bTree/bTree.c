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
        fseek(binFile, BT_HEADER_SIZE + (rrn * BT_PAGE_SIZE), SEEK_SET);

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

    fseek(binFile, BT_HEADER_SIZE + (rrn * BT_PAGE_SIZE), SEEK_SET);

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

// is this even needed
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

static int allocate_page(FILE *binFile, BTHeader *header)
{
    if (header->top != -1)
    {
        int rrn = header->top;

        BTPage *page = read_page(binFile, rrn);
        header->top = page->next;

        free(page);

        header->numNodes++;
        return rrn;
    }

    header->numNodes++;
    return header->nextRRN++;
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
    // todo: change with binary search(?)
    while (pos < page->keyCount && page->keys[pos].searchKey < key.searchKey)
    {
        pos++;
    }

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
        *rightChildRRN = allocate_page(binFile, header);

        if (page->nodeType == ROOT)
        {
            page->nodeType = (page->subPages[0] == -1) ? LEAF : INTERMEDIARY;
            newPage->nodeType = page->nodeType;
        }

        write_page(binFile, page, currentRRN);
        write_page(binFile, newPage, *rightChildRRN);

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

    // if tree is empty, insert a new page as root (which for some reason needs to have nodeType = -1, since it's also a LEAF)
    if (header->rootNode == -1)
    {
        BTPage *root = create_page();
        root->nodeType = LEAF; // see comment above
        root->keys[0] = key;
        root->keyCount = 1;

        header->rootNode = allocate_page(binFile, header);

        write_page(binFile, root, header->rootNode);

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

        header->rootNode = allocate_page(binFile, header);
        write_page(binFile, newRoot, header->rootNode);

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

static BTPage *get_page_successor(FILE *binFile, int rightChildRRN, int *successorRRN)
{
    int rrn = rightChildRRN;
    BTPage *current = read_page(binFile, rrn);

    while (current->nodeType != LEAF)
    {
        int nextRRN = current->subPages[0];
        free(current);
        rrn = nextRRN;
        current = read_page(binFile, rrn);
    }

    *successorRRN = rrn;
    return current;
}

static void clear_trailing_slots(BTPage *page)
{
    for (int i = page->keyCount; i < TREE_ORDER - 1; i++)
        page->keys[i] = (BTKey){-1, -1};

    for (int i = page->keyCount + 1; i < TREE_ORDER; i++)
        page->subPages[i] = -1;
}

static void redistribute_left(FILE *binFile, BTPage *parent, int childIdx, int parentRRN, int childRRN)
{
    int siblingRRN = parent->subPages[childIdx - 1];
    BTPage *sibling = read_page(binFile, siblingRRN);
    BTPage *child = read_page(binFile, childRRN);

    // build working arrays: sibling + separator + child
    int total = sibling->keyCount + 1 + child->keyCount;
    BTKey workingKeys[TREE_ORDER * 2];
    int workingChildren[TREE_ORDER * 2 + 1];

    for (int i = 0; i < sibling->keyCount; i++)
    {
        workingKeys[i] = sibling->keys[i];
        workingChildren[i] = sibling->subPages[i];
    }
    workingChildren[sibling->keyCount] = sibling->subPages[sibling->keyCount];

    workingKeys[sibling->keyCount] = parent->keys[childIdx - 1]; // separator

    for (int i = 0; i < child->keyCount; i++)
    {
        workingKeys[sibling->keyCount + 1 + i] = child->keys[i];
        workingChildren[sibling->keyCount + 1 + i] = child->subPages[i];
    }
    workingChildren[total] = child->subPages[child->keyCount];

    // promoted key index — left gets one more if uneven
    int promoIdx = total / 2;

    // left (sibling) gets keys[0..promoIdx-1]
    sibling->keyCount = 0;
    for (int i = 0; i < promoIdx; i++)
    {
        sibling->keys[i] = workingKeys[i];
        sibling->subPages[i] = workingChildren[i];
        sibling->keyCount++;
    }
    sibling->subPages[promoIdx] = workingChildren[promoIdx];

    // promoted key goes to parent
    parent->keys[childIdx - 1] = workingKeys[promoIdx];

    // right (child) gets keys[promoIdx+1..total-1]
    child->keyCount = 0;
    child->subPages[0] = workingChildren[promoIdx + 1];
    for (int i = promoIdx + 1; i < total; i++)
    {
        child->keys[child->keyCount] = workingKeys[i];
        child->subPages[child->keyCount + 1] = workingChildren[i + 1];
        child->keyCount++;
    }

    // clear remaining slots
    clear_trailing_slots(sibling);
    clear_trailing_slots(child);

    write_page(binFile, parent, parentRRN);
    write_page(binFile, sibling, siblingRRN);
    write_page(binFile, child, childRRN);

    free(sibling);
    free(child);
}

static void redistribute_right(FILE *binFile, BTPage *parent, int childIdx, int parentRRN, int childRRN)
{
    int siblingRRN = parent->subPages[childIdx + 1];
    BTPage *sibling = read_page(binFile, siblingRRN);
    BTPage *child = read_page(binFile, childRRN);

    // build working arrays with all keys: child + separator + sibling
    int total = child->keyCount + 1 + sibling->keyCount;
    BTKey workingKeys[TREE_ORDER * 2];
    int workingChildren[TREE_ORDER * 2 + 1];

    for (int i = 0; i < child->keyCount; i++)
    {
        workingKeys[i] = child->keys[i];
        workingChildren[i] = child->subPages[i];
    }
    workingChildren[child->keyCount] = child->subPages[child->keyCount];

    workingKeys[child->keyCount] = parent->keys[childIdx];       // separator
    workingChildren[child->keyCount + 1] = sibling->subPages[0]; // not used for leaves but needed

    for (int i = 0; i < sibling->keyCount; i++)
    {
        workingKeys[child->keyCount + 1 + i] = sibling->keys[i];
        workingChildren[child->keyCount + 2 + i] = sibling->subPages[i + 1];
    }

    // find promoted key index (most uniform distribution, left gets one more if uneven)
    int promoIdx = total / 2;

    // left node gets keys[0..promoIdx-1]
    child->keyCount = 0;
    for (int i = 0; i < promoIdx; i++)
    {
        child->keys[i] = workingKeys[i];
        child->subPages[i] = workingChildren[i];
        child->keyCount++;
    }
    child->subPages[promoIdx] = workingChildren[promoIdx];

    // promoted key goes to parent
    parent->keys[childIdx] = workingKeys[promoIdx];

    // right node gets keys[promoIdx+1..total-1]
    sibling->keyCount = 0;
    sibling->subPages[0] = workingChildren[promoIdx + 1];
    for (int i = promoIdx + 1; i < total; i++)
    {
        sibling->keys[sibling->keyCount] = workingKeys[i];
        sibling->subPages[sibling->keyCount + 1] = workingChildren[i + 1];
        sibling->keyCount++;
    }

    // clear remaining slots
    clear_trailing_slots(child);
    clear_trailing_slots(sibling);

    write_page(binFile, parent, parentRRN);
    write_page(binFile, child, childRRN);
    write_page(binFile, sibling, siblingRRN);

    free(sibling);
    free(child);
}

static void deallocate_page(FILE *binFile, BTHeader *header, int rrn)
{
    BTPage *page = read_page(binFile, rrn);
    page->removed = '1';

    page->next = header->top;
    header->top = rrn;

    write_page(binFile, page, rrn);
    header->numNodes--;

    free(page);
}

static void merge_children(FILE *binFile, BTHeader *header, BTPage *parent, int childIdx, int parentRRN, int childRRN)
{
    int siblingRRN = parent->subPages[childIdx - 1];
    BTPage *sibling = read_page(binFile, siblingRRN);
    BTPage *child = read_page(binFile, childRRN);

    if (!sibling || !child)
    {
        if (sibling)
            free(sibling);
        if (child)
            free(child);
        return;
    }

    sibling->keys[sibling->keyCount] = parent->keys[childIdx - 1];
    sibling->subPages[sibling->keyCount + 1] = child->subPages[0];
    sibling->keyCount++;

    // copy child keys into sibling
    for (int i = 0; i < child->keyCount; i++)
    {
        sibling->keys[sibling->keyCount] = child->keys[i];
        sibling->subPages[sibling->keyCount + 1] = child->subPages[i + 1];
        sibling->keyCount++;
    }

    // remove separator from parent
    for (int i = childIdx - 1; i < parent->keyCount - 1; i++)
    {
        parent->keys[i] = parent->keys[i + 1];
        parent->subPages[i + 1] = parent->subPages[i + 2];
    }
    parent->keyCount--;

    parent->subPages[parent->keyCount + 1] = -1;

    clear_trailing_slots(sibling);
    clear_trailing_slots(parent);

    write_page(binFile, sibling, siblingRRN);
    write_page(binFile, parent, parentRRN);

    deallocate_page(binFile, header, childRRN);

    free(sibling);
    free(child);
}

static int has_spare_keys(FILE *binFile, int rrn)
{
    BTPage *page = read_page(binFile, rrn);
    int result = page->keyCount > MIN_OCCUPANCY;
    free(page);
    return result;
}

static RemoveResult handle_underflow(FILE *binFile, BTHeader *header, BTPage *parent, int childIdx, int childRRN, int parentRRN)
{
    if (childIdx < parent->keyCount && has_spare_keys(binFile, parent->subPages[childIdx + 1]))
    {
        redistribute_right(binFile, parent, childIdx, parentRRN, childRRN);
        return REMOVED;
    }

    if (childIdx > 0 && has_spare_keys(binFile, parent->subPages[childIdx - 1]))
    {
        redistribute_left(binFile, parent, childIdx, parentRRN, childRRN);
        return REMOVED;
    }

    if (childIdx > 0)
        merge_children(binFile, header, parent, childIdx, parentRRN, childRRN);
    else
        merge_children(binFile, header, parent, childIdx + 1, parentRRN, parent->subPages[childIdx + 1]);

    return (parent->keyCount < MIN_OCCUPANCY) ? REMOVED_UNDERFLOW : REMOVED;
}

static RemoveResult remove_loop(FILE *binFile, BTHeader *header, int currentRRN, int searchKey)
{
    BTPage *page = read_page(binFile, currentRRN);
    if (!page)
        return REMOVE_ERROR;

    int pos = binary_search(page, searchKey);

    // key found in non-leaf: swap with successor then remove it
    if (pos >= 0 && page->nodeType != LEAF)
    {
        int successorRRN;
        BTPage *successor = get_page_successor(binFile, page->subPages[pos + 1], &successorRRN);
        int successorKey = successor->keys[0].searchKey;
        page->keys[pos] = successor->keys[0];
        free(successor);
        write_page(binFile, page, currentRRN);

        int childRRN = page->subPages[pos + 1];
        RemoveResult result = remove_loop(binFile, header, childRRN, successorKey);

        if (result == REMOVED_UNDERFLOW)
        {
            free(page);
            page = read_page(binFile, currentRRN);
            result = handle_underflow(binFile, header, page, pos + 1, childRRN, currentRRN);
        }

        free(page);
        return result;
    }

    // key found in leaf: remove directly
    if (pos >= 0)
    {
        for (int i = pos; i < page->keyCount - 1; i++)
            page->keys[i] = page->keys[i + 1];
        page->keys[page->keyCount - 1] = (BTKey){-1, -1};
        page->keyCount--;

        clear_trailing_slots(page);

        write_page(binFile, page, currentRRN);
        RemoveResult result = (page->keyCount < MIN_OCCUPANCY) ? REMOVED_UNDERFLOW : REMOVED;
        free(page);
        return result;
    }

    // key not in this page: recurse into child
    int childIdx = -pos - 1;

    if (page->nodeType == LEAF)
    {
        free(page);
        return NOT_FOUND;
    }

    int childRRN = page->subPages[childIdx];
    RemoveResult result = remove_loop(binFile, header, childRRN, searchKey);

    if (result == REMOVED_UNDERFLOW)
    {
        free(page);
        page = read_page(binFile, currentRRN);
        result = handle_underflow(binFile, header, page, childIdx, childRRN, currentRRN);
    }

    free(page);
    return result;
}

Status remove_key(FILE *binFile, BTHeader *header, int searchKey)
{
    if (!binFile || !header || header->rootNode == -1)
        return FAILURE;

    RemoveResult result = remove_loop(binFile, header, header->rootNode, searchKey);

    if (result == NOT_FOUND || result == REMOVE_ERROR)
        return FAILURE;

    // case 6: root lost its last key; shrink tree height
    if (result == REMOVED_UNDERFLOW)
    {
        BTPage *root = read_page(binFile, header->rootNode);
        if (root->keyCount == 0)
        {
            int oldRoot = header->rootNode;
            header->rootNode = root->subPages[0];

            if (header->rootNode != -1)
            {
                BTPage *newRoot = read_page(binFile, header->rootNode);
                newRoot->nodeType = ROOT;
                write_page(binFile, newRoot, header->rootNode);
                free(newRoot);
            }

            free(root);
            deallocate_page(binFile, header, oldRoot);
        }
        else
            free(root);
    }

    return SUCCESS;
}