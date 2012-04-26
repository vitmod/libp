#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "dts_enc.h"
    
static dtsenc_info_t dtsenc_info;
int dtsenc_init()
{
    memset(&dtsenc_info,0,sizeof(dtsenc_info_t));
    //get dts info
    dtsenc_info.dts_flag=0;
    //shaoshuai xujian add init
    return 0;
}
int dtsenc_start()
{
    if(dtsenc_info.state!=INITTED)
        return -1;
    //start thread
    dtsenc_info.state=ACTIVE;
    return 0;
}
int dtsenc_pause()
{
    if(dtsenc_info.state!=ACTIVE)
        return -1;
    dtsenc_info.state=PAUSED;
    return 0;
}
int dtsenc_resume()
{
    if(dtsenc_info.state!=PAUSED)
        return -1;
    dtsenc_info.state=ACTIVE;
    return 0;
}
int dtsenc_stop()
{
    if(dtsenc_info.state<INITTED)
            return -1;
    dtsenc_info.state=STOPPED;
    //jone the thread
}
int dtsenc_release();
{
    if(dtsenc_info.state!=STOPPED)
            return -1;
    dtsenc_info.state=TERMINATED;
     //release other --shaoshuai  xujian  add release
}

static void *dts_enc_loop()
{
    while(1)
    {
        switch(dtsenc_info.state)
        {
            case ACTIVE:
                break;
            case PAUSED:
                usleep(100000);
                continue;
            case STOPPED:
                goto quit_loop;
            default:
                goto err;
          }
          //shaoshuai --non_block
          //enc_data();
          //write data();
    }
 quit_loop:
    pthread_exit(NULL);
    return 0;
 err:
    pthread_exit(NULL);
    return -1;
}


