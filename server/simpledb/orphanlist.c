/*
 * orphanlist.c - a sorted doubly-linked list which holds orphan data
 */

#include "orphanlist.h"
#include "simpledb.h"
#include <stdlib.h>

void olist_create(olist** list)
{
    *list = (olist*) malloc(sizeof(olist));
    (*list)->node_count = 0;
    (*list)->head = NULL;
    (*list)->tail = NULL;
}

void olist_destroy(olist* list)
{
    onode* c_node;

    c_node = list->head;
    for(; c_node != NULL; c_node = c_node->next)
    {
       _olist_destroy_node(c_node); 
    }

    free(list);
}

void olist_insert(olist* list, struct sdb_orphan* orphan)
{
    onode* c_node = NULL;
    onode* new_node = NULL;

    /* new list, add first node */
    if( list->node_count == 0 )
    {
        _olist_create_node(&(list->head), orphan);
        list->tail = list->head;
    }
    else
    {
        /* sorted from smallest to greatest */
        c_node = list->head;
        for(; c_node != NULL; c_node = c_node->next)
        {
            if( orphan->size > c_node->orphan.size )
                continue;
            else
                break;
        }

        _olist_create_node(&new_node, orphan);
        
        /* add it before the head (making it the new head) */
        if( c_node == list->head )
        {
            new_node->next = list->head;
            list->head->prev = new_node;
            list->head = new_node;
        }
        else if( !c_node ) /* add to end of list */
        {
            new_node->prev = list->head;
            list->head->next = new_node;
            list->tail = new_node;
        }
        else /* update links */
        {
            new_node->prev = c_node->prev;
            new_node->next = c_node;
            new_node->prev->next = new_node;
            c_node->prev = new_node;
        }
    }

    list->node_count++;
}

void olist_remove(olist* list, onode* node)
{
    onode* c_node = list->head;
    for(; c_node != NULL; c_node = c_node->next)
    {
        if( node == c_node )
        {
            if(node->prev)
            {
                if( node == list->tail )
                    list->tail = node->prev;
                node->prev->next = node->next;
            }

            if(node->next)
            {
                if(node == list->head)
                    list->head = node->next;
                node->next->prev = node->prev;
            }

            if(!(node->next) && !(node->prev))
                list->tail = list->head = NULL;

            _olist_destroy_node(node);
            list->node_count--;
        }
    }
}

void _olist_create_node(onode** node, struct sdb_orphan* orphan)
{
    (*node) = (onode*) malloc(sizeof(onode));
    (*node)->next = NULL;
    (*node)->prev = NULL;
    (*node)->orphan.offset = orphan->offset;
    (*node)->orphan.size = orphan->size;
}

void _olist_destroy_node(onode* node)
{
    free(node);
}
