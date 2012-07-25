#ifndef _H_ORPHANLIST_
#define _H_ORPHANLIST_
#include "simpledb.h"

struct olist_node;
typedef struct olist_node
{
    struct sdb_orphan orphan;

    struct olist_node* next;
    struct olist_node* prev;
} onode;

typedef struct orphan_list
{
    int node_count;
    onode* head;
    onode* tail;
} olist;

void olist_insert(olist* list, struct sdb_orphan* orphan);
void olist_remove(olist* list, onode* node);
void olist_create(olist** list);
void olist_destroy(olist* list);

void _olist_create_node(onode** node, struct sdb_orphan* orphan);
void _olist_destroy_node(onode* node);
#endif
