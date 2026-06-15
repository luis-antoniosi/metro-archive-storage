#ifndef BTREE_MODIFY_H
#define BTREE_MODIFY_H

#include <stdio.h>
#include "types.h"
#include "bTree.h"

Status insert_index_key(FILE *indexFile, IndexHeader *header, IndexKey key);

Status remove_index_key(FILE *indexFile, IndexHeader *header, int removedKey);

#endif