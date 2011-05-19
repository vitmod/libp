/************************************************
 * name :player_cache_file.c
 * function :cache file manager
 * data     :2010.2.4
 *************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <player.h>
#include <string.h>
#include "player_priv.h"

#include "player_cache_file.h"

#include "libavformat/aviolpcache.h"

static int cache_client_open(const char *url,int64_t filesize)
{
	return (int)cachefile_open(url,filesize,0);
}


static int cache_client_read(unsigned long id,int64_t off,char *buf,int size)
{
	return cachefile_read((struct cache_file *)id,off,buf,size);
}

static int cache_next_valid_bytes(unsigned long id,int64_t off,int size)
{
	return cachefile_searce_valid_bytes((struct cache_file *)id,off,size);
}


static int cache_client_write(unsigned long id,int64_t off,char *buf,int size)
{
	return cachefile_write((struct cache_file *)id,off,buf,size);
}

static int cache_client_close(unsigned long id)
{
	return cachefile_close((struct cache_file *)id);
}


static struct cache_client client=
{
	/*
	int (*cache_read)(unsigned long id,int64_t off,char *buf,int size);
	int (*cache_write)(unsigned long id,int64_t off,char *buf,int size);
	unsigned long (*cache_open)(char *url,int64_t filesize);
	int (*cache_close)(unsigned long id);
	*/
	.cache_open=cache_client_open,
	.cache_write=cache_client_write,
	.cache_read=cache_client_read,
	.cache_close=cache_client_close,
	.cache_next_valid_bytes=cache_next_valid_bytes,
};

void cache_system_init()
{
	//aviolp_register_cache_system(&client);
}




