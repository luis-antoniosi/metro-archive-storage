#include <stdio.h>
#include <stdlib.h>
#include "bTree.h"
#include "modify.h"
#include "binFile/indexFile.h"  // for IndexHeader type

// Insert and delete

// Helper, static functions for insertion

// used in insert_loop and insert_index_key
/**
 * @brief Allocates a page RRN for a new IndexPage
 * 
 * Reuses a page if header->top is not -1, otherwise just increments header->nextRRN.
 * 
 * @param indexFile Index file containing all pages
 * @param header indexFile's header
 * @return Header's nextRRN
 */
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
/**
 * @brief Simply orders a page, adding an insertKey and putting all its keys and subPages in a sorted state.
 */
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
/**
 * @brief Splits a full page into two, returning the promoted key
 * 
 * @param page Page that'll be splitted, left half will stay here
 * @param newPage Other page, right half of the split values will go here
 * @param insertKey Key to be inserted
 * @param insertRRN RRN of the key to be inserted
 * @return IndexKey that was promoted in the process
 */
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
/**
 * @brief Recursive function that navigates the b-tree and inserts a key into it, 
 * handling splitting and promotion.
 * 
 * @param indexFile File containing all indices
 * @param header indexFile's header
 * @param currentRRN RRN of the current page
 * @param key Key to be inserted
 * @param promotedKey Key to be promoted
 * @param rightChildRRN currentRRN's rightChild's RRN
 * @return ERROR, PROMOTION or NO_PROMOTION
 */
static InsertResult insert_loop(FILE *indexFile, IndexHeader *header, int currentRRN, IndexKey key, IndexKey *promotedKey, int *rightChildRRN)
{
    IndexPage *page = read_index_page(indexFile, currentRRN);
    if (!page)
        return ERROR;

    if (page->removed == RECORD_REMOVED)
    {
        free(page);
        return ERROR;
    }

    // checking if the key is already in the tree
    int pos = binary_index_search(page, key.searchKey);

    if (pos >= 0)   
    {
        // key already exists (idx >= 0)
        free(page);
        return ERROR;   
    }

    // getting the subPage idx of where the key would be, since it doesn't exist
    pos = -pos - 1;

    InsertResult result;
    // page is a leaf, insertion should happen normally
    if (page->nodeType == LEAF || page->subPages[0] == -1)
    {
        *promotedKey = key;
        *rightChildRRN = -1;    // there is no right child
        result = PROMOTION;     // PROMOTION just to pass future check
    }
    else    // not a leaf, so we loop until finding one
        result = insert_loop(indexFile, header, page->subPages[pos], key, promotedKey, rightChildRRN);

    // there was no PROMOTION, return
    if (result != PROMOTION)
    {
        free(page);
        return result;
    }

    // If there was a PROMOTION, check for the page's keyCount
    if (page->keyCount < TREE_ORDER - 1)
    {
        // if there's space, simply insert it into the page and write it again.
        order_page(page->keys, page->subPages, *promotedKey, *rightChildRRN, page->keyCount);
        page->keyCount++;

        write_index_page(indexFile, page, currentRRN);
        free(page);

        return NO_PROMOTION;
    }
    else
    {
        // if there's no space,
        IndexPage *newPage = create_index_page();
        newPage->nodeType = page->nodeType;

        // we split the current page, and allocate a page for the right child
        *promotedKey = split_index_page(page, newPage, *promotedKey, *rightChildRRN);
        *rightChildRRN = allocate_page(indexFile, header);

        // If it we split a root, assign the correct nodeTypes
        if (page->nodeType == ROOT)
        {
            page->nodeType = (page->subPages[0] == -1) ? LEAF : INTERMEDIARY;
            newPage->nodeType = page->nodeType;
        }

        write_index_page(indexFile, page, currentRRN);
        write_index_page(indexFile, newPage, *rightChildRRN);

        free(page);
        free(newPage);
        return PROMOTION;   // return PROMOTION
    }
}

/**
 * @brief Inserts a key into the index file's b-tree, as long as it doesn't already exist.
 * Traverses the tree to find the correct insertion point, splits and does all the operations when needed.
 * 
 * @param indexFile Index file where the key will be inserted into.
 * @param header indexFile's header
 * @param key Key to be inserted
 * @return SUCCESS or FAILURE
 */
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

    IndexKey promotedKey;
    int rightChildRRN = -1;

    InsertResult result = insert_loop(indexFile, header, header->rootNode, key, &promotedKey, &rightChildRRN);

    if (result == ERROR)
        return FAILURE;

    // checking for root split. if it occurrred, need to allocate a new root node, increasing the height by 1 
    if (result == PROMOTION)
    {
        IndexPage *newRoot = create_index_page();
        newRoot->nodeType = ROOT;
        newRoot->keys[0] = promotedKey;
        newRoot->keyCount = 1;
        newRoot->subPages[0] = header->rootNode;    // left pointer to root
        newRoot->subPages[1] = rightChildRRN;       // right pointer to the other split part

        header->rootNode = allocate_page(indexFile, header);
        write_index_page(indexFile, newRoot, header->rootNode);

        free(newRoot);
    }

    return SUCCESS;
}

// Helper, static functions for deletion

// used in remove_loop
/**
 * @brief Finds the in-order succesor key of a page, given its right child's RRN.
 * 
 * @param indexFile File containing all indices
 * @param rightChildRRN RRN of the page's right child
 * @param[out] successorRRN RRN of the successor's page
 * @return Pointer to the IndexPage containing the successor
 */
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
/**
 * @brief Clears all values of a page.
 * 
 * @param page Page to be cleared
 */
static void clear_trailing_slots(IndexPage *page)
{
    for (int i = page->keyCount; i < TREE_ORDER - 1; i++)
        page->keys[i] = (IndexKey){-1, -1};

    for (int i = page->keyCount + 1; i < TREE_ORDER; i++)
        page->subPages[i] = -1;
}

// used in handle_underflow
/**
 * @brief Resolves underflow in "child" by pulling a key down from the parent,
 * and rotating a key up from its left sibling.
 * 
 * @param indexFile File containing all indices
 * @param parent Parent page, where the key will be pulled down from
 * @param childIdx subPages index of the child
 * @param parentRRN RRN of the parent
 * @param childRRN RRN of the child
 */
static void redistribute_left(FILE *indexFile, IndexPage *parent, int childIdx, int parentRRN, int childRRN)
{
    // sibling is to the left (-1)
    int siblingRRN = parent->subPages[childIdx - 1];
    IndexPage *sibling = read_index_page(indexFile, siblingRRN);
    IndexPage *child = read_index_page(indexFile, childRRN);

    // creating "working" arrays where the process of combining and splitting will happen in an evenly order
    int total = sibling->keyCount + 1 + child->keyCount;
    IndexKey workingKeys[TREE_ORDER * 2];
    int workingChildren[TREE_ORDER * 2 + 1];

    // copy left sibling's keys and subPages into working arrays
    for (int i = 0; i < sibling->keyCount; i++)
    {
        workingKeys[i] = sibling->keys[i];
        workingChildren[i] = sibling->subPages[i];
    }
    workingChildren[sibling->keyCount] = sibling->subPages[sibling->keyCount];

    // parent's key is inserted between sibling and child
    workingKeys[sibling->keyCount] = parent->keys[childIdx - 1]; // separator

    // copy the underflowed child's content into working arrays
    for (int i = 0; i < child->keyCount; i++)
    {
        workingKeys[sibling->keyCount + 1 + i] = child->keys[i];
        workingChildren[sibling->keyCount + 1 + i] = child->subPages[i];
    }
    workingChildren[total] = child->subPages[child->keyCount];

    // promoted key's index 
    int promoIdx = total / 2;

    // giving the lower half to the sibling
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

    // giving the upper half to the child
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
/**
 * @brief Resolves underflow in "child" by pulling a key down from the parent,
 * and rotating a key up from its right sibling.
 *
 * @param indexFile File containg all indices
 * @param parent Parent page, where the key will be pulled down from
 * @param childIdx subPages index of the child
 * @param parentRRN RRN of the parent
 * @param childRRN RRN of the child
 */
static void redistribute_right(FILE *indexFile, IndexPage *parent, int childIdx, int parentRRN, int childRRN)
{
    // sibling is to the right (+1)
    int siblingRRN = parent->subPages[childIdx + 1];
    IndexPage *sibling = read_index_page(indexFile, siblingRRN);
    IndexPage *child = read_index_page(indexFile, childRRN);

    // creating "working" arrays where the process of combining and splitting will happen in an evenly order
    int total = child->keyCount + 1 + sibling->keyCount;
    IndexKey workingKeys[TREE_ORDER * 2];
    int workingChildren[TREE_ORDER * 2 + 1];

    // copy the underflowed child's content into working arrays
    for (int i = 0; i < child->keyCount; i++)
    {
        workingKeys[i] = child->keys[i];
        workingChildren[i] = child->subPages[i];
    }
    workingChildren[child->keyCount] = child->subPages[child->keyCount];

    // parent's key is inserted between child and sibling
    workingKeys[child->keyCount] = parent->keys[childIdx];
    workingChildren[child->keyCount + 1] = sibling->subPages[0];

    // copy right sibling's content into working array
    for (int i = 0; i < sibling->keyCount; i++)
    {
        workingKeys[child->keyCount + 1 + i] = sibling->keys[i];
        workingChildren[child->keyCount + 2 + i] = sibling->subPages[i + 1];
    }

    // promoted key's index
    int promoIdx = total / 2;

    // giving the lower half to the child
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

    // giving the upper half to the sibling
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

/**
 * @brief Marks a page as logically deleted, 
 * updates the header's top with the page's rrn.
 * 
 * @param indexFile File containing all indices
 * @param header indexFile's header
 * @param rrn RRN of the page that'll be deallocated
 */
static void deallocate_page(FILE *indexFile, IndexHeader *header, int rrn)
{
    IndexPage *page = read_index_page(indexFile, rrn);
    page->removed = RECORD_REMOVED;

    page->next = header->top;
    header->top = rrn;

    write_index_page(indexFile, page, rrn);
    header->numNodes--;

    free(page);
}

// used in handle_underflow
/**
 * @brief Merges and underflowed "child" with its left sibling.
 * 
 * @param indexFile File containing all indices
 * @param header indexFile's header
 * @param parent Parent page, where the key will be pulled down from
 * @param childIdx subPages index of the child
 * @param parentRRN RRN of the parent
 * @param childRRN RRN of the child
 */
static void merge_children(FILE *indexFile, IndexHeader *header, IndexPage *parent, int childIdx, int parentRRN, int childRRN)
{
    // left sibling
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

    // pull down parent's key
    sibling->keys[sibling->keyCount] = parent->keys[childIdx - 1];
    // moving the child's pointer since we just pulled the parent's key
    sibling->subPages[sibling->keyCount + 1] = child->subPages[0];
    sibling->keyCount++;

    // copy child's keys into sibling
    for (int i = 0; i < child->keyCount; i++)
    {
        sibling->keys[sibling->keyCount] = child->keys[i];
        sibling->subPages[sibling->keyCount + 1] = child->subPages[i + 1];
        sibling->keyCount++;
    }

    // remove key from parent, shift everything left
    for (int i = childIdx - 1; i < parent->keyCount - 1; i++)
    {
        parent->keys[i] = parent->keys[i + 1];
        parent->subPages[i + 1] = parent->subPages[i + 2];
    }
    parent->keyCount--;

    // set the trailing subPage to -1
    parent->subPages[parent->keyCount + 1] = -1;

    clear_trailing_slots(sibling);
    clear_trailing_slots(parent);

    write_index_page(indexFile, sibling, siblingRRN);
    write_index_page(indexFile, parent, parentRRN);

    // deallocate the empty child page
    deallocate_page(indexFile, header, childRRN);

    free(sibling);
    free(child);
}

// used in handle_underflow
/**
 * @brief Checks if a page is above MIN_OCCUPANCY at a specific RRN
 * 
 * @param indexFile File with all indices
 * @param rrn RRN of the page
 * @return 1 if it is above MIN_OCCUPANCY, 0 if not
 */
static int has_spare_keys(FILE *indexFile, int rrn)
{
    IndexPage *page = read_index_page(indexFile, rrn);
    int result = page->keyCount > MIN_OCCUPANCY;
    free(page);
    return result;
}

/**
 * @brief Handles underflow in a child page after key removal.
 * 
 * Tries right sibling redistribuition first, then left, then merges.
 * Merge always keeps keys in the left page and deallocates the right.
 * 
 * @param indexFile File containing all indices
 * @param header indexFile's header
 * @param parent Parent page
 * @param childIdx Index of the child page
 * @param childRRN RRN of the child page
 * @param parentRRN RRN of the parent page
 * @return REMOVED_UNDERFLOW if the merge caused the parent to underflow, REMOVED if not.
 */
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
/**
 * @brief Recursive function that navigates a b-tree to remove a key, 
 * handling underflow when needed.
 * 
 * @param indexFile File containing all indices
 * @param header indexFile's header
 * @param currentRRN RRN of the current page
 * @param searchKey Key to be deleted
 * @return REMOVE_ERROR, REMOVED, NOT_FOUND, REMOVED_UNDERFLOW
 */
static RemoveResult remove_loop(FILE *indexFile, IndexHeader *header, int currentRRN, int searchKey)
{
    IndexPage *page = read_index_page(indexFile, currentRRN);
    if (!page)
        return REMOVE_ERROR;

    int pos = binary_index_search(page, searchKey);

    // case 1 -> key found in non-leaf: swap with successor then remove it recursively with its duplicate successor
    if (pos >= 0 && page->nodeType != LEAF)
    {
        int successorRRN = 0;
        IndexPage *successor = get_page_successor(indexFile, page->subPages[pos + 1], &successorRRN);
        int successorKey = successor->keys[0].searchKey;

        // overwrite key with its successor
        page->keys[pos] = successor->keys[0];
        free(successor);
        write_index_page(indexFile, page, currentRRN);

        // recurse into the right tree to eliminate the duplicated value
        int childRRN = page->subPages[pos + 1];
        RemoveResult result = remove_loop(indexFile, header, childRRN, successorKey);

        // if an underflow happens, resolve it
        if (result == REMOVED_UNDERFLOW)
        {
            free(page);
            page = read_index_page(indexFile, currentRRN);
            result = handle_underflow(indexFile, header, page, pos + 1, childRRN, currentRRN);
        }

        free(page);
        return result;
    }

    // case 2 -> key found in leaf: remove directly
    if (pos >= 0)
    {
        // shift keys left
        for (int i = pos; i < page->keyCount - 1; i++)
            page->keys[i] = page->keys[i + 1];
        page->keys[page->keyCount - 1] = (IndexKey){-1, -1};
        page->keyCount--;

        clear_trailing_slots(page);
        write_index_page(indexFile, page, currentRRN);

        //
        RemoveResult result = (page->keyCount < MIN_OCCUPANCY) ? REMOVED_UNDERFLOW : REMOVED;
        free(page);
        return result;
    }

    // key not in this page: recurse into child, using the idx trick fror binary_index_search
    int childIdx = -pos - 1;

    if (page->nodeType == LEAF)
    {
        // reached a leaf, there was no key
        free(page);
        return NOT_FOUND;
    }

    int childRRN = page->subPages[childIdx];
    RemoveResult result = remove_loop(indexFile, header, childRRN, searchKey);

    // if there was any underflow, handle it
    if (result == REMOVED_UNDERFLOW)
    {
        free(page);
        page = read_index_page(indexFile, currentRRN);
        result = handle_underflow(indexFile, header, page, childIdx, childRRN, currentRRN);
    }

    free(page);
    return result;
}

/**
 * @brief Removes a key from the index file's b-tree, as long as it exists.
 * Uses remove_loop to recursively travel the b-tree, 
 * then handles the case of the root being empty after a merge 
 * 
 * @param indexFile Index file where the key will be removed from.
 * @param header indexFile's header
 * @param searchKey Key to remove
 * @return SUCCESS or FAILURE
 */
Status remove_index_key(FILE *indexFile, IndexHeader *header, int searchKey)
{
    if (!indexFile || !header || header->rootNode == -1)
        return FAILURE;

    RemoveResult result = remove_loop(indexFile, header, header->rootNode, searchKey);

    if (result == NOT_FOUND || result == REMOVE_ERROR)
        return FAILURE;

    // root lost its last key as a result of a merge; shrink tree's height
    if (result == REMOVED_UNDERFLOW)
    {
        IndexPage *root = read_index_page(indexFile, header->rootNode);
        if (root->keyCount == 0)
        {
            int oldRoot = header->rootNode;
            // single remaining child pointer
            header->rootNode = root->subPages[0];

            if (header->rootNode != -1)
            {
                IndexPage *newRoot = read_index_page(indexFile, header->rootNode);
                newRoot->nodeType = ROOT;
                write_index_page(indexFile, newRoot, header->rootNode);
                free(newRoot);
            }

            free(root);
            // old root is deallocated
            deallocate_page(indexFile, header, oldRoot);
        }
        else
            free(root);
    }

    return SUCCESS;
}