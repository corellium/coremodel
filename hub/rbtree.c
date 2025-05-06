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

#include "rbtree.h"

#define CONTAINER(node) ((node) ? ((void *)((uintptr_t)(node) - tree->nodeoff)) : NULL)
#define NODE(container) ((rbnode_t *)((uintptr_t)(container) + tree->nodeoff))

void rbtree_setup(rbtree_t *tree, intptr_t nodeoff, rbcompare_func_t compare, void *param)
{
    tree->root = NULL;
    tree->nodeoff = nodeoff;
    tree->compare = compare;
    tree->compkey = NULL;
    tree->param = param;
}

void rbtree_setup_with_key(rbtree_t *tree, intptr_t nodeoff, rbcompare_func_t compare, rbcompare_func_t compkey, void *param)
{
    tree->root = NULL;
    tree->nodeoff = nodeoff;
    tree->compare = compare;
    tree->compkey = compkey;
    tree->param = param;
}

static rbnode_t **rbtree_find_ref(rbtree_t *tree, void *key, rbnode_t **p)
{
    rbnode_t **vv = &tree->root, *pp = NULL;
    int i;

    while(*vv != NULL) {
        i = tree->compare(tree->param, CONTAINER(*vv), key);
        if(!i)
            break;
        pp = *vv;
        vv = (i > 0) ? &(*vv)->l : &(*vv)->r;
    }
    if(p)
        *p = pp;
    return vv;
}

static rbnode_t **rbtree_find_ref_key(rbtree_t *tree, void *key, rbnode_t **p)
{
    rbnode_t **vv = &tree->root, *pp = NULL;
    int i;

    while(*vv != NULL) {
        i = tree->compkey(tree->param, CONTAINER(*vv), key);
        if(!i)
            break;
        pp = *vv;
        vv = (i > 0) ? &(*vv)->l : &(*vv)->r;
    }
    if(p)
        *p = pp;
    return vv;
}

void *rbtree_find(rbtree_t *tree, void *key)
{
    rbnode_t **vv;
    if(tree->compkey)
        vv = rbtree_find_ref_key(tree, key, NULL);
    else
        vv = rbtree_find_ref(tree, key, NULL);
    if(!vv)
        return NULL;
    return CONTAINER(*vv);
}

static void rbtree_rotate(rbnode_t **root, rbnode_t *vv)
{
    rbnode_t **pp, *qq, *qqxx;

    qq = vv->p;
    if(qq == NULL)
        return;
    if(qq->p != NULL) {
        qqxx = qq->p;
        pp = (qq == qqxx->l) ? &qqxx->l : &qqxx->r;
    } else
        pp = root;

    vv->p = qq->p;
    *pp = vv;
    if(vv == qq->l) {
        qq->l = vv->r;
        if(qq->l != NULL) {
            qqxx = qq->l;
            qqxx->p = qq;
        }
        vv->r = qq;
    } else {
        qq->r = vv->l;
        if(qq->r != NULL) {
            qqxx = qq->r;
            qqxx->p = qq;
        }
        vv->l = qq;
    }
    qq->p = vv;
}

static void rbtree_insert_repair(rbnode_t **root, rbnode_t *nn)
{
    rbnode_t *pp, *gg, *uu;

recurse_repair:
    pp = nn->p;
    if(pp == NULL) {
        nn->rb = RB_BLACK;
        return;
    }

    if(pp->rb == RB_BLACK)
        return;

    gg = pp->p; /* must exist because parent is red */
    uu = (gg->l == pp) ? gg->r : gg->l; /* may be a leaf - check u before access */
    if(uu != NULL && uu->rb == RB_RED) {
        pp->rb = RB_BLACK;
        uu->rb = RB_BLACK;
        gg->rb = RB_RED;
        nn = pp->p;
        goto recurse_repair;
    }

    /* if inside child of parent, rotate out */
    if((pp->l == nn && gg->r == pp) || (pp->r == nn && gg->l == pp)) {
        rbtree_rotate(root, nn);
        nn = pp;
        pp = nn->p;
    }

    pp->rb = RB_BLACK;
    gg->rb = RB_RED;
    rbtree_rotate(root, pp);
}

int rbtree_insert(rbtree_t *tree, void *item)
{
    rbnode_t *pp, **vv, *ii = NODE(item);

    vv = rbtree_find_ref(tree, item, &pp);
    if(*vv != NULL)
        return 1;

    *vv = ii;
    ii->l = ii->r = NULL;
    ii->p = pp;
    ii->rb = RB_RED;

    rbtree_insert_repair(&tree->root, ii);

    pp = ii;
    while(pp->p != NULL)
        pp = pp->p;
    tree->root = pp;

    return 0;
}

static void rbtree_exchange(rbnode_t **root, rbnode_t *mm, rbnode_t *nn)
{
    rbnode_t *mp, *np, **mpcp, **npcp;
    uint8_t rb;

    mp = mm->p;
    np = nn->p;
    mpcp = (mp == NULL) ? root : (mp->l == mm) ? &mp->l : &mp->r;
    npcp = (np == NULL) ? root : (np->l == nn) ? &np->l : &np->r;

    /* exchange parents */
    *mpcp = nn; nn->p = mp;
    *npcp = mm; mm->p = np;

    /* exchange children */
    mp = mm->l; mm->l = nn->l; nn->l = mp;
    np = mm->r; mm->r = nn->r; nn->r = np;
    if(mm->l != NULL) mm->l->p = mm;
    if(mm->r != NULL) mm->r->p = mm;
    if(nn->l != NULL) nn->l->p = nn;
    if(nn->r != NULL) nn->r->p = nn;

    /* exchange color */
    rb = mm->rb; mm->rb = nn->rb; nn->rb = rb;
}

void rbtree_remove(rbtree_t *tree, void *item)
{
    rbnode_t *nn, *pp, **pcp, *cc, *ss, *ssll, *ssrr;
    rbnode_t *ii = NODE(item);

    /* find leftmost node of right child */
    if(ii->r != NULL) {
        nn = ii->r;
        while(nn->l != NULL)
            nn = nn->l;
        rbtree_exchange(&tree->root, ii, nn);
    }

    pp = ii->p;
    pcp = (pp == NULL) ? &tree->root : (pp->l == ii) ? &pp->l : &pp->r;
    cc = (ii->l != NULL) ? ii->l : ii->r;
    *pcp = cc;
    if(cc)
        cc->p = pp;
    /* ii is now out of the tree for good */

    if(ii->rb == RB_RED) { /* removing red node - no consequence */
        ii->rb = RB_IDLE;
        return;
    }
    ii->rb = RB_IDLE;

    nn = cc;

    if(nn && nn->rb == RB_RED) { /* removed black node, make child black */
        nn->rb = RB_BLACK;
        return;
    }

recurse_remove: /* must have valid p/pp and n/nn */
    if(!pp)
        return;

    ss = (pp->l == nn) ? pp->r : pp->l;
    if(ss && ss->rb == RB_RED) {
        pp->rb = RB_RED;
        ss->rb = RB_BLACK;
        rbtree_rotate(&tree->root, ss);
        ss = (pp->l == nn) ? pp->r : pp->l;
    }

    if(!ss)
        return;

    if(pp->rb == RB_BLACK && ss->rb == RB_BLACK) {
        ssll = ss->l;
        ssrr = ss->r;
        if((!ssll || ssll->rb == RB_BLACK) && (!ssrr || ssrr->rb == RB_BLACK)) {
            ss->rb = RB_RED;
            nn = pp;
            pp = nn->p;
            goto recurse_remove;
        }
    }

    if(pp->rb == RB_RED && ss->rb == RB_BLACK) {
        ssll = ss->l;
        ssrr = ss->r;
        if((!ssll || ssll->rb == RB_BLACK) && (!ssrr || ssrr->rb == RB_BLACK)) {
            ss->rb = RB_RED;
            pp->rb = RB_BLACK;
            return;
        }
    }

    if(ss->rb == RB_BLACK) {
        ssll = ss->l;
        ssrr = ss->r;
        if(pp->l == nn) {
            if((!ssrr || ssrr->rb == RB_BLACK) && ssll && ssll->rb == RB_RED) {
                ss->rb = RB_RED;
                ssll->rb = RB_BLACK;
                ss = ss->l;
                rbtree_rotate(&tree->root, ss);
            }
        } else {
            if((!ssll || ssll->rb == RB_BLACK) && ssrr && ssrr->rb == RB_RED) {
                ss->rb = RB_RED;
                ssrr->rb = RB_BLACK;
                ss = ss->r;
                rbtree_rotate(&tree->root, ss);
            }
        }
    }

    if(ss) {
        ss->rb = pp->rb;
        pp->rb = RB_BLACK;
        if(pp->l == nn) {
            ssrr = ss->r;
            ssrr->rb = RB_BLACK;
        } else {
            ssll = ss->l;
            ssll->rb = RB_BLACK;
        }
        rbtree_rotate(&tree->root, ss);
    }
}

void *rbtree_next(rbtree_t *tree, void *item)
{
    rbnode_t *pp, *ii;

    if(!item) {
        if(!tree->root)
            return NULL;
        for(pp=tree->root; pp->l; pp=pp->l) ;
        return CONTAINER(pp);
    }

    ii = NODE(item);
    if(ii->r != NULL) {
        ii = ii->r;
        while(ii->l != NULL)
            ii = ii->l;
        return CONTAINER(ii);
    }

    if(ii->p == NULL)
        return NULL;

    pp = ii->p;
    while(pp->r == ii) {
        ii = pp;
        if(ii->p == NULL)
            return NULL;
        pp = ii->p;
    }
    return CONTAINER(pp);
}

void *rbtree_prev(rbtree_t *tree, void *item)
{
    rbnode_t *pp, *ii;

    if(!item) {
        if(!tree->root)
            return NULL;
        for(pp=tree->root; pp->r; pp=pp->r) ;
        return CONTAINER(pp);
    }

    ii = NODE(item);
    if(ii->l != NULL) {
        ii = ii->l;
        while(ii->r != NULL)
            ii = ii->r;
        return CONTAINER(ii);
    }

    if(ii->p == NULL)
        return NULL;

    pp = ii->p;
    while(pp->l == ii) {
        ii = pp;
        if(ii->p == NULL)
            return NULL;
        pp = ii->p;
    }
    return CONTAINER(pp);
}

void *rbtree_rangestart(rbtree_t *tree, void *key)
{
    rbnode_t *pp, **vv;

    if(tree->compkey)
        vv = rbtree_find_ref_key(tree, key, &pp);
    else
        vv = rbtree_find_ref(tree, key, &pp);

    if(*vv != NULL)
        return CONTAINER(*vv);

    if(vv == &tree->root)
        return NULL;

    if(vv == &pp->l)
        return CONTAINER(pp);

    return rbtree_next(tree, CONTAINER(pp));
}

void *rbtree_rangeend(rbtree_t *tree, void *key)
{
    rbnode_t *pp, **vv;

    if(tree->compkey)
        vv = rbtree_find_ref_key(tree, key, &pp);
    else
        vv = rbtree_find_ref(tree, key, &pp);

    if(*vv != NULL)
        return CONTAINER(*vv);

    if(vv == &tree->root)
        return NULL;

    if(vv == &pp->r)
        return CONTAINER(pp);

    return rbtree_prev(tree, CONTAINER(pp));
}

static void rbtree_walk_recurse(rbtree_t *tree, rbnode_t *node, rbwalk_func_t walk, void *param)
{
    if(node->l)
        rbtree_walk_recurse(tree, node->l, walk, param);
    walk(param, CONTAINER(node));
    if(node->r)
        rbtree_walk_recurse(tree, node->r, walk, param);
}

void rbtree_walk(rbtree_t *tree, rbwalk_func_t walk, void *param)
{
    if(tree->root)
        rbtree_walk_recurse(tree, tree->root, walk, param);
}

static void rbtree_free_recurse(rbtree_t *tree, rbnode_t **node, rbdestruct_func_t destruct, void *param)
{
    if((*node)->l)
        rbtree_free_recurse(tree, &(*node)->l, destruct, param);
    if((*node)->r)
        rbtree_free_recurse(tree, &(*node)->r, destruct, param);
    destruct(param, CONTAINER(*node));
    *node = NULL;
}

void rbtree_free(rbtree_t *tree, rbdestruct_func_t destruct, void *param)
{
    if(tree->root)
        rbtree_free_recurse(tree, &tree->root, destruct, param);
}
