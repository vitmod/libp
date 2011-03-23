#ifndef _PLAYER_ITEMLIST_H_
#define _PLAYER_ITEMLIST_H_

#include "list.h"
#include <pthread.h>


struct item {
    struct list_head list;
    unsigned long item_data;
    unsigned long flags;
};

struct itemlist {
    struct list_head list;
    int item_count;
    pthread_mutex_t list_mutex;
};

typedef void(*data_free_fun)(void *);

#endif

