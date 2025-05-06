/*
 *
 * This file contains confidential information of Corellium LLC.
 * Unauthorized reproduction or distribution of this file is subject
 * to civil and criminal penalties.
 *
 * Copyright (C) 2019-21 Corellium LLC
 * All rights reserved.
 *
 */

#ifndef _RBTREE_H
#define _RBTREE_H

#include <stddef.h>
#include <stdint.h>

#define RB_IDLE         0
#define RB_RED          1
#define RB_BLACK        2

/* <0: a is less than b, =0: a is equal to b, >0: a is greater than b */
typedef int (*rbcompare_func_t)(void *param, void *a, void *b);

typedef void (*rbdestruct_func_t)(void *param, void *a);
typedef void (*rbwalk_func_t)(void *param, void *a);

typedef struct rbnode {
    struct rbnode *p, *l, *r;
    uint8_t rb;
} rbnode_t;

typedef struct {
    rbnode_t *root;
    intptr_t nodeoff;
    rbcompare_func_t compare, compkey;
    void *param;
} rbtree_t;

/* fills out a rbtree structure based on passed information: nodeoff is byte offset of rbnode_t inside each entry */
void rbtree_setup(rbtree_t *tree, intptr_t nodeoff, rbcompare_func_t compare, void *param);
void rbtree_setup_with_key(rbtree_t *tree, intptr_t nodeoff, rbcompare_func_t compare, rbcompare_func_t compkey, void *param);
/* finds an exact match to specified key, or NULL if not found */
void *rbtree_find(rbtree_t *tree, void *key);
/* inserts an entry into tree and returns 0; if already present, does not insert and returns 1 */
int rbtree_insert(rbtree_t *tree, void *item);
/* removes item from tree (do not call on entries not in tree) */
void rbtree_remove(rbtree_t *tree, void *item);
/* finds the smallest entry not smaller than key, NULL if not found */
void *rbtree_rangestart(rbtree_t *tree, void *key);
/* finds the greatest entry not greater than key, NULL if not found */
void *rbtree_rangeend(rbtree_t *tree, void *key);
/* finds the smallest entry greater than item */
void *rbtree_next(rbtree_t *tree, void *item);
/* finds the largest entry smaller than item */
void *rbtree_prev(rbtree_t *tree, void *item);
/* in-order walks all tree nodes using a supplied walk function, which must not modify the tree */
void rbtree_walk(rbtree_t *tree, rbwalk_func_t walk, void *param);
/* releases all tree nodes using a supplied destructor */
void rbtree_free(rbtree_t *tree, rbdestruct_func_t destruct, void *param);

#endif
