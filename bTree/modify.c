#include <stdio.h>
#include <stdlib.h>
#include "bTree.h"
#include "modify.h"

// Insert and delete

// Helper, static functions for insertion

// used in insert_loop and insert_index_key
static int allocate_page(FILE *indexFile, IndexHeader *header)
{
    if (header->top != -1)
    {
        int rrn = header->top;

        IndexPage *page = read_index_page(indexFile, rrn);
        header->top = page->next;

        free(page);

        header->numNodes++;
        return rrn;
    }

    header->numNodes++;
    return header->nextRRN++;
}

// used in split_index_page and insert_loop
static void order_page(IndexKey *workingKeys, int *workingPages, IndexKey insertKey, int insertRRN, int count)
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

// used in insert_loop
static IndexKey split_index_page(IndexPage *page, IndexPage *newPage, IndexKey insertKey, int insertRRN)
{
    IndexKey workingKeys[TREE_ORDER];
    int workingPages[TREE_ORDER + 1];

    for (int i = 0; i < TREE_ORDER - 1; i++)
        workingKeys[i] = page->keys[i];
    for (int i = 0; i < TREE_ORDER; i++)
        workingPages[i] = page->subPages[i];

    order_page(workingKeys, workingPages, insertKey, insertRRN, TREE_ORDER - 1);

    int promoIdx = TREE_ORDER / 2;
    IndexKey promoKey = workingKeys[promoIdx];

    for (int i = 0; i < TREE_ORDER - 1; i++)
    {
        if (i < promoIdx)
            page->keys[i] = workingKeys[i];
        else
            page->keys[i] = (IndexKey){-1, -1};
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

// used here for recursion and in insert_index_key
static InsertResult insert_loop(FILE *indexFile, IndexHeader *header, int currentRRN, IndexKey key, IndexKey *promotedKey, int *rightChildRRN)
{
    IndexPage *page = read_index_page(indexFile, currentRRN);
    if (!page)
        return ERROR;

    // checking if the key is already in the tree
    int pos = binary_index_search(page, key.searchKey);

    if (pos >= 0)   
    {
        // key already exists (idx >= 0)
        free(page);
        return ERROR;   
    }

    pos = -pos - 1;

    InsertResult result;
    if (page->nodeType == LEAF || page->subPages[0] == -1)
    {
        *promotedKey = key;
        *rightChildRRN = -1;
        result = PROMOTION;
    }
    else
        result = insert_loop(indexFile, header, page->subPages[pos], key, promotedKey, rightChildRRN);

    if (result != PROMOTION)
    {
        free(page);
        return result;
    }

    if (page->keyCount < TREE_ORDER - 1)
    {
        order_page(page->keys, page->subPages, *promotedKey, *rightChildRRN, page->keyCount);
        page->keyCount++;

        write_index_page(indexFile, page, currentRRN);
        free(page);

        return NO_PROMOTION;
    }
    else
    {
        IndexPage *newPage = create_index_page();
        newPage->nodeType = page->nodeType;

        *promotedKey = split_index_page(page, newPage, *promotedKey, *rightChildRRN);
        *rightChildRRN = allocate_page(indexFile, header);

        if (page->nodeType == ROOT)
        {
            page->nodeType = (page->subPages[0] == -1) ? LEAF : INTERMEDIARY;
            newPage->nodeType = page->nodeType;
        }

        write_index_page(indexFile, page, currentRRN);
        write_index_page(indexFile, newPage, *rightChildRRN);

        free(page);
        free(newPage);
        return PROMOTION;
    }
}

Status insert_index_key(FILE *indexFile, IndexHeader *header, IndexKey key)
{
    if (!indexFile || !header)
        return FAILURE;

    // if tree is empty, insert a new page as root (which for some reason needs to have nodeType = -1, since it's also a LEAF)
    if (header->rootNode == -1)
    {
        IndexPage *root = create_index_page();
        root->nodeType = LEAF; // see comment above
        root->keys[0] = key;
        root->keyCount = 1;

        header->rootNode = allocate_page(indexFile, header);

        write_index_page(indexFile, root, header->rootNode);

        free(root);
        return SUCCESS;
    }

    // otherwise
    IndexKey promotedKey;
    int rightChildRRN = -1;

    InsertResult result = insert_loop(indexFile, header, header->rootNode, key, &promotedKey, &rightChildRRN);

    if (result == ERROR)
        return FAILURE;

    if (result == PROMOTION)
    {
        IndexPage *newRoot = create_index_page();
        newRoot->nodeType = ROOT;
        newRoot->keys[0] = promotedKey;
        newRoot->keyCount = 1;
        newRoot->subPages[0] = header->rootNode;
        newRoot->subPages[1] = rightChildRRN;

        header->rootNode = allocate_page(indexFile, header);
        write_index_page(indexFile, newRoot, header->rootNode);

        free(newRoot);
    }

    return SUCCESS;
}

// Helper, static functions for deletion

// used in remove_loop
static IndexPage *get_page_successor(FILE *indexFile, int rightChildRRN, int *successorRRN)
{
    int rrn = rightChildRRN;
    IndexPage *current = read_index_page(indexFile, rrn);

    while (current->nodeType != LEAF)
    {
        int nextRRN = current->subPages[0];
        free(current);
        rrn = nextRRN;
        current = read_index_page(indexFile, rrn);
    }

    *successorRRN = rrn;
    return current;
}

// used in redistribute_left, redistribute_right, merge_children and remove_loop
static void clear_trailing_slots(IndexPage *page)
{
    for (int i = page->keyCount; i < TREE_ORDER - 1; i++)
        page->keys[i] = (IndexKey){-1, -1};

    for (int i = page->keyCount + 1; i < TREE_ORDER; i++)
        page->subPages[i] = -1;
}

// used in handle_underflow
static void redistribute_left(FILE *indexFile, IndexPage *parent, int childIdx, int parentRRN, int childRRN)
{
    int siblingRRN = parent->subPages[childIdx - 1];
    IndexPage *sibling = read_index_page(indexFile, siblingRRN);
    IndexPage *child = read_index_page(indexFile, childRRN);

    // build working arrays: sibling + separator + child
    int total = sibling->keyCount + 1 + child->keyCount;
    IndexKey workingKeys[TREE_ORDER * 2];
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

    write_index_page(indexFile, parent, parentRRN);
    write_index_page(indexFile, sibling, siblingRRN);
    write_index_page(indexFile, child, childRRN);

    free(sibling);
    free(child);
}

// used in hande_underflow
static void redistribute_right(FILE *indexFile, IndexPage *parent, int childIdx, int parentRRN, int childRRN)
{
    int siblingRRN = parent->subPages[childIdx + 1];
    IndexPage *sibling = read_index_page(indexFile, siblingRRN);
    IndexPage *child = read_index_page(indexFile, childRRN);

    // build working arrays with all keys: child + separator + sibling
    int total = child->keyCount + 1 + sibling->keyCount;
    IndexKey workingKeys[TREE_ORDER * 2];
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

    write_index_page(indexFile, parent, parentRRN);
    write_index_page(indexFile, child, childRRN);
    write_index_page(indexFile, sibling, siblingRRN);

    free(sibling);
    free(child);
}

// used in merge_children and remove_index_key
static void deallocate_page(FILE *indexFile, IndexHeader *header, int rrn)
{
    IndexPage *page = read_index_page(indexFile, rrn);
    page->removed = '1';

    page->next = header->top;
    header->top = rrn;

    write_index_page(indexFile, page, rrn);
    header->numNodes--;

    free(page);
}

// used in handle_underflow
static void merge_children(FILE *indexFile, IndexHeader *header, IndexPage *parent, int childIdx, int parentRRN, int childRRN)
{
    int siblingRRN = parent->subPages[childIdx - 1];
    IndexPage *sibling = read_index_page(indexFile, siblingRRN);
    IndexPage *child = read_index_page(indexFile, childRRN);

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

    write_index_page(indexFile, sibling, siblingRRN);
    write_index_page(indexFile, parent, parentRRN);

    deallocate_page(indexFile, header, childRRN);

    free(sibling);
    free(child);
}

// used in handle_underflow
static int has_spare_keys(FILE *indexFile, int rrn)
{
    IndexPage *page = read_index_page(indexFile, rrn);
    int result = page->keyCount > MIN_OCCUPANCY;
    free(page);
    return result;
}

static RemoveResult handle_underflow(FILE *indexFile, IndexHeader *header, IndexPage *parent, int childIdx, int childRRN, int parentRRN)
{
    if (childIdx < parent->keyCount && has_spare_keys(indexFile, parent->subPages[childIdx + 1]))
    {
        redistribute_right(indexFile, parent, childIdx, parentRRN, childRRN);
        return REMOVED;
    }

    if (childIdx > 0 && has_spare_keys(indexFile, parent->subPages[childIdx - 1]))
    {
        redistribute_left(indexFile, parent, childIdx, parentRRN, childRRN);
        return REMOVED;
    }

    if (childIdx > 0)
        merge_children(indexFile, header, parent, childIdx, parentRRN, childRRN);
    else
        merge_children(indexFile, header, parent, childIdx + 1, parentRRN, parent->subPages[childIdx + 1]);

    return (parent->keyCount < MIN_OCCUPANCY) ? REMOVED_UNDERFLOW : REMOVED;
}

// used here for recursion and in remove_index_key
static RemoveResult remove_loop(FILE *indexFile, IndexHeader *header, int currentRRN, int searchKey)
{
    IndexPage *page = read_index_page(indexFile, currentRRN);
    if (!page)
        return REMOVE_ERROR;

    int pos = binary_index_search(page, searchKey);

    // key found in non-leaf: swap with successor then remove it
    if (pos >= 0 && page->nodeType != LEAF)
    {
        int successorRRN;
        IndexPage *successor = get_page_successor(indexFile, page->subPages[pos + 1], &successorRRN);
        int successorKey = successor->keys[0].searchKey;
        page->keys[pos] = successor->keys[0];
        free(successor);
        write_index_page(indexFile, page, currentRRN);

        int childRRN = page->subPages[pos + 1];
        RemoveResult result = remove_loop(indexFile, header, childRRN, successorKey);

        if (result == REMOVED_UNDERFLOW)
        {
            free(page);
            page = read_index_page(indexFile, currentRRN);
            result = handle_underflow(indexFile, header, page, pos + 1, childRRN, currentRRN);
        }

        free(page);
        return result;
    }

    // key found in leaf: remove directly
    if (pos >= 0)
    {
        for (int i = pos; i < page->keyCount - 1; i++)
            page->keys[i] = page->keys[i + 1];
        page->keys[page->keyCount - 1] = (IndexKey){-1, -1};
        page->keyCount--;

        clear_trailing_slots(page);

        write_index_page(indexFile, page, currentRRN);
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
    RemoveResult result = remove_loop(indexFile, header, childRRN, searchKey);

    if (result == REMOVED_UNDERFLOW)
    {
        free(page);
        page = read_index_page(indexFile, currentRRN);
        result = handle_underflow(indexFile, header, page, childIdx, childRRN, currentRRN);
    }

    free(page);
    return result;
}

Status remove_index_key(FILE *indexFile, IndexHeader *header, int searchKey)
{
    if (!indexFile || !header || header->rootNode == -1)
        return FAILURE;

    RemoveResult result = remove_loop(indexFile, header, header->rootNode, searchKey);

    if (result == NOT_FOUND || result == REMOVE_ERROR)
        return FAILURE;

    // case 6: root lost its last key; shrink tree height
    if (result == REMOVED_UNDERFLOW)
    {
        IndexPage *root = read_index_page(indexFile, header->rootNode);
        if (root->keyCount == 0)
        {
            int oldRoot = header->rootNode;
            header->rootNode = root->subPages[0];

            if (header->rootNode != -1)
            {
                IndexPage *newRoot = read_index_page(indexFile, header->rootNode);
                newRoot->nodeType = ROOT;
                write_index_page(indexFile, newRoot, header->rootNode);
                free(newRoot);
            }

            free(root);
            deallocate_page(indexFile, header, oldRoot);
        }
        else
            free(root);
    }

    return SUCCESS;
}