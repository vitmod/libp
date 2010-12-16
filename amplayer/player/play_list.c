#include <stdio.h>
#include <sys/time.h> 
#include <time.h> 
#include <pthread.h>
#include <player.h>

#include "player_priv.h"
#include "play_list.h"


#if 0
static struct list_head play_list;
static pthread_mutex_t list_mutex;
#define IS_SAME(s,file) ((strlen(file)==strlen(s)) && (memcmp(s,file,strlen(file))==0))

#define FILE_SEPRATOR	';'
int init_play_list(void)
{	
	 pthread_mutex_init(&list_mutex,NULL);
	INIT_LIST_HEAD(&play_list);
	return 0;
}
static play_list_t *alloc_list_struct(char * file)
{
	play_list_t *list;
	list=MALLOC(sizeof(play_list_t)+strlen(file));
	return list;
}
static void free_list_struct(play_list_t * list)
{
	FREE(list);
}
int play_list_add_file(char* file)
{
	play_list_t *plist;
	plist=alloc_list_struct(file);
	if(plist!=NULL)
	{
    	strcpy(plist->file,file);
    	pthread_mutex_lock(&list_mutex);
    	list_add_tail(&plist->list,&play_list);
    	pthread_mutex_unlock(&list_mutex);
	}
	return 0;
}
play_list_t *search_file_on_list(struct  list_head* head,char *file)
{
	play_list_t *plist=NULL;
	pthread_mutex_lock(&list_mutex);
    if(plist != NULL)
	{
        list_for_each_entry(plist,head,list)
	    {
    	    if(IS_SAME(plist->file,file))
    		    break;
	    }
    }
	pthread_mutex_unlock(&list_mutex);
	return plist;
}

int play_list_del_file(char*file)
{
	play_list_t *plist=NULL;
	search_file_on_list(&play_list,file);
	if(plist)
	{
    	pthread_mutex_lock(&list_mutex);
    	list_del(&plist->list);
    	pthread_mutex_unlock(&list_mutex);
    	free_list_struct(plist);
    	return 0;
	}
	return -1;	
}
/*file format =="file;file2;file3;file4"*/
int play_list_add_files(char*files)
{
	char *file;
	char *fptr,*eptr;
	file=MALLOC(MAX_PATH);
	if(file==NULL)
		return PLAYER_NOMEM;
	for(fptr=files;fptr[0]!='\0';fptr=eptr+1)
		{
		char *tptr;
		for(tptr=fptr;tptr[0]!='\0' && tptr[0]!=FILE_SEPRATOR;tptr++);
		eptr=tptr;
		if((eptr-fptr)>MAX_PATH)
			continue;
		MEMCPY(file,files,eptr-fptr);
		file[eptr-fptr]='\0';
		play_list_add_file(file);
		if(tptr[0]=='\0')
			break;
		}	

	return 0;
}
#else
play_list_t play_list;

static int play_list_add_file(play_list_t *list,char *file_path)
{
	if(!list || !file_path)
		return -1;
	
	unsigned int in_idx = list->in_list_index;
	if(in_idx < MAX_LIST_NUM)
	{
		list->list[in_idx]= MALLOC(strlen(file_path)+1);
		if(list->list[in_idx])
		{
			strcpy(list->list[in_idx],file_path);				
			list->list_num ++;
			log_print("add [%d:in_idx %d] file:%s\n",list->list_num,list->in_list_index,list->list[in_idx]);
			in_idx ++;		
			list->in_list_index = in_idx;
			if(in_idx == MAX_LIST_NUM)
				log_print("[%s:%d]play  list is full !\n",__FUNCTION__,__LINE__);
			return 0;
		}
		else
			log_error("malloc [%d] bytes for list[%d]  failed \n",strlen(file_path)+1, in_idx);
	}
	return -2;
}
int play_list_get_line(ByteIOContext *s,char *line,int size)
{
	int ch;
	char *q;
	#define GET_CHAR_TRY_MAX	(100)
	int i=0;
	q = line;
	while(1)
	{	
get_char:
		ch = url_fgetc(s);
		if (ch < 0)
		{
			if(i<GET_CHAR_TRY_MAX)
			{
				i ++;
				goto get_char;
			}
			return -1;
		}
		if (ch == '\n') {
		    /* process line */
		    if (q > line && q[-1] == '\r')
				q--;
			*q = '\0';
			log_print("play_list_get_line:%s\n",line);
			return 0;
		} else {
		    if ((q - line) < size - 1)
		        *q++ = ch;
		}		
	}
}

int play_list_init()
{	
	MEMSET(&play_list, 0, sizeof(play_list_t));
	return 0;
}

int play_list_parser_m3u(const char *filename)
{
	int err;
	ByteIOContext *s;
	char line[MAX_PATH];
	char *pline;
	int num=0;
	DEBUG_PN();
	///int url_fopen(ByteIOContext **s, const char *filename, int flags)
	if((err=url_fopen(&s,filename,URL_RDONLY))<0)
	{
		return err;
	}
	MEMSET(&play_list, 0,  sizeof(play_list_t));
	while(play_list_get_line(s,line,sizeof(line))==0)
	{
		pline=line;
		while(*pline==' ' ) pline++;
		if(strlen(pline)>0 && pline[0]!='#')
		{
			pline[strlen(pline)]= '\0';
			if(play_list_add_file(&play_list,pline)==0)			
				num++;			
			else			
				break;			
		}
	}	
	url_fclose(s);
	DEBUG_PN();
	return num;
}

play_list_t *player_get_playlist()
{
	return &play_list;
}

char * play_list_get_file(void)
{
	play_list_t *plist = player_get_playlist();
	if(plist)
	{
		unsigned int out_idx;
		char *file_name = NULL;
		out_idx = plist->out_list_index;		
		if(out_idx < plist->list_num)			
		{
			file_name = plist->list[out_idx];
			log_print("Get a file :%s from list[%d]\n", file_name,out_idx);
			out_idx ++;
		}
		return file_name;
	}
	log_print("[%s:%d]Get a file Failed\n",__FUNCTION__,__LINE__);
	return NULL;
}

void play_list_release()
{
	unsigned int i;
	play_list_t*plist = player_get_playlist();
	log_print("Enter Play List Rlease!\n");
	for(i = 0; i < plist->list_num; i ++)
	{
		if(plist->list[i])
		{
			FREE(plist->list[i]);
			plist->list[i] = NULL;
		}
	}
	log_print("Exit Play List Rlease!\n");
}

#endif

