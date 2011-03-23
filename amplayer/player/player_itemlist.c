/********************************************
 * name : player_itemlis.c
 * function: item  fifo manage  for muti threads
 * date     : 2011.3.23
 * author :zhouzhi
 ********************************************/

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "player_itemlist.h"
#include "player_priv.h"

int itemlist_init(struct itemlist *itemlist)
{
    itemlist->item_count = 0;
    INIT_LIST_HEAD(&itemlist->list);
    pthread_mutex_init(&itemlist->list_mutex, NULL);
    return 0;
}

static inline struct item * item_alloc(void) {
    return MALLOC(sizeof(struct item));
}


static inline void item_free(struct item *item)
{
    return FREE(item);
}


int itemlist_push(struct itemlist *itemlist, struct item *item)
{
    pthread_mutex_lock(&itemlist->list_mutex);
    list_add_tail(&itemlist->list, &item->list);
    itemlist->item_count++;
    pthread_mutex_unlock(&itemlist->list_mutex);
    return 0;
}

struct item *  itemlist_pop(struct itemlist *itemlist) {
    struct item *item = NULL;
    struct list_head *list = NULL;

    pthread_mutex_lock(&itemlist->list_mutex);
    if (!list_empty(&itemlist->list)) {
        list = itemlist->list.next;
        item = list_entry(list, struct item, list);
        list_del(list);
        itemlist->item_count--;
    }
    pthread_mutex_unlock(&itemlist->list_mutex);
    return item;
}

struct item * itemlist_peek(struct itemlist *itemlist) {
    struct item *item = NULL;
    struct list_head *list = NULL;

    pthread_mutex_lock(&itemlist->list_mutex);
    if (!list_empty(&itemlist->list)) {
        list = itemlist->list.next;
        item = list_entry(list, struct item, list);
    }
    pthread_mutex_unlock(&itemlist->list_mutex);
    return item;
}

struct item * itemlist_reverse_peek(struct itemlist *itemlist) {
    struct item *item = NULL;
    struct list_head *list = NULL;

    pthread_mutex_lock(&itemlist->list_mutex);
    if (!list_empty(&itemlist->list)) {
        list = itemlist->list.prev;
        item = list_entry(list, struct item, list);
    }
    pthread_mutex_unlock(&itemlist->list_mutex);
    return item;
}

int  itemlist_clean(struct itemlist *itemlist, data_free_fun free_fun)
{
    struct item *item = NULL;
    struct list_head *list, *tmplist;
    pthread_mutex_lock(&itemlist->list_mutex);
    list_for_each_safe(list, tmplist, &itemlist->list) {
        item = list_entry(list, struct item, list);
        if (free_fun != NULL && item->item_data != 0) {
            free_fun((void *)item->item_data);
        }
        item_free(item);
        itemlist->item_count--;
    }
    pthread_mutex_unlock(&itemlist->list_mutex);
    return 0;
}


int itemlist_push_data(struct itemlist *itemlist, unsigned long data)
{
    struct item *item = item_alloc();
    if (item == NULL) {
        return PLAYER_NOMEM;
    }
    item->item_data = data;
    itemlist_push(itemlist, item);
    return 0;
}
int  itemlist_pop_data(struct itemlist *itemlist, unsigned long *data)
{
    struct item *item = NULL;
    item = itemlist_pop(itemlist);
    if (item != NULL) {
        *data = item->item_data;
        item_free(item);
        return 0;
    } else {
        return -1;
    }
}


int itemlist_peek_data(struct itemlist *itemlist, unsigned long *data)
{
    struct item *item = NULL;
    item = itemlist_peek(itemlist);
    if (item != NULL) {
        *data = item->item_data;
        return 0;
    } else {
        return -1;
    }
}


int itemlist_reverse_peek_data(struct itemlist *itemlist, unsigned long *data)
{
    struct item *item = NULL;
    item = itemlist_reverse_peek(itemlist);
    if (item != NULL) {
        *data = item->item_data;
        return 0;
    } else {
        return -1;
    }
}

int itemlist_clean_data(struct itemlist *itemlist, data_free_fun free_fun)
{
    return itemlist_clean(itemlist, free_fun);
}






