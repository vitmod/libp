/********************************************
 * name             : queue.c
 * function     : for basic buf queue manage
 * initialize date     : 2012.8.31
 * author       :zh.zhou@amlogic.com
 ********************************************/
#include "queue.h"

int queue_init(bufqueue_t *queue, int flags)
{
    memset(queue, 0, sizeof(bufheader_t));
    INIT_LIST_HEAD(&queue->list);

    return 0;
}
bufqueue_t *queue_alloc(int flags)
{
    bufqueue_t *queue;
    queue = malloc(sizeof(bufqueue_t));
    if (queue == NULL) {
        return NULL;
    }
    queue_init(queue, 0);
    return queue;
}
int queue_free(bufqueue_t *queue)
{
    bufheader_t *buf;
    buf = queue_bufget(queue);
    while (buf != NULL) {
        queue_buffree(buf);
        buf = queue_bufget(queue);
    }
    return 0;
}

bufheader_t *queue_bufalloc(int datasize)
{
    bufheader_t* buf;

    buf = (bufheader_t*)malloc(sizeof(bufheader_t));
    if (buf == NULL) {
        return NULL;
    }
    memset(buf, 0, sizeof(bufheader_t));
    buf->pbuf = malloc(datasize);
    if (!buf->pbuf) {
        free(buf);
        return NULL;
    }
    buf->bufsize = datasize;
    buf->data_start = buf->pbuf;
    return buf;
}

int queue_bufrealloc(bufheader_t *buf, int datasize)
{
    char *oldbuf = buf->pbuf;
    free(oldbuf);
    buf->pbuf = malloc(datasize);
    if (!buf->pbuf) {
        buf->pbuf = oldbuf;
        return -1;
    }
    buf->bufsize = datasize;
    return 0;
}

int queue_buffree(bufheader_t*buf)
{
    free(buf->pbuf);
    free(buf);
    return 0;
}

int queue_bufpush(bufqueue_t *queue, bufheader_t *buf)
{
    list_add_tail(&buf->list, &queue->list);
    if (queue->datasize <= 0) {
        queue->startpos = buf->pos;
    }
    queue->datasize += buf->bufdatalen;
    return 0;
}

bufheader_t *queue_bufget(bufqueue_t *queue)
{
    bufheader_t *buf;
    if (list_empty(&queue->list)) {
        return NULL;
    }
    buf = list_first_entry(&queue->list, bufheader_t, list);
    list_del(&buf->list);
    queue->startpos += buf->bufdatalen;
    queue->datasize -= buf->bufdatalen;
    return buf;
}

bufheader_t *queue_bufpeek(bufqueue_t *queue)
{
    bufheader_t *buf;
    if (list_empty(&queue->list)) {
        return NULL;
    }
    buf = list_first_entry(&queue->list, bufheader_t, list);
    return buf;
}

int queue_bufpeeked_partdatasize(bufqueue_t *queue, bufheader_t *buf, int size)
{
    buf->data_start += size;
    return 0;
}
int queue_bufdatasize(bufqueue_t *queue)
{
    return queue->datasize;
}

int64_t queue_bufstartpos(bufqueue_t *queue)
{
    return queue->startpos;
}


