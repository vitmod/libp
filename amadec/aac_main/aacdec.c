#include "adec.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static AVPacket packet;

static DECLARE_ALIGNED(16, uint8_t, rbuf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]);

static DECLARE_ALIGNED(16, uint8_t, dec_buf1[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]);
static DECLARE_ALIGNED(16, uint8_t, dec_buf2[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]);
static uint8_t *audio_buf = NULL;

static adec_feeder_t * aac_feeder = NULL;
static AVCodec * aac_codec = NULL;
static AVCodecContext * aac_ctx = NULL;

int AAC_init(adec_feeder_t *feeder, void * arg)
{
    if(feeder == NULL || arg == NULL){
        return -1;
    }

    aac_ctx =  (AVCodecContext*)arg;

    aac_feeder = feeder;
    aac_feeder->data_width = 16;

    aac_codec = avcodec_find_decoder_by_name("aac");
    if (!aac_codec){
        log_print(LOG_ERR, "avcodec_finde_decoder_by_name(\"aac\") error!\n");
        return -1;
    }

    if (avcodec_open(aac_ctx, aac_codec) < 0){
        log_print(LOG_ERR, "avcodec_open() error!\n");
        return -1;
    }

    av_init_packet(&packet);
    packet.size = 0;
    packet.data = NULL;

    log_print(LOG_INFO, "[%s,%d]codec init ok!\n", __func__, __LINE__);
    return 0;
}

int AAC_release(void)
{
    avcodec_close(aac_ctx);
    return 0;
}

static inline int AAC_read(char *buf,int size)
{
    return aac_feeder->get_bytes(buf, size);
}

int AAC_decode_frame(char *buf,int len,struct frame_fmt *fmt)
{
    int decoded;
    int data_size;

    if(packet.size  <= 0){
        int ret = 0;

        packet.size = 4096;
        ret = AAC_read(rbuf, packet.size);
        packet.data = rbuf;

        if(ret <= 0){
            packet.data = NULL;
            packet.size = 0;
            return 0;
        }
    }

    data_size = sizeof(dec_buf1);

    /*
    log_print(LOG_ERR, "avcodec_decode_audio3:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n", packet.data[0], packet.data[1],packet.data[2], packet.data[3],
            packet.data[4], packet.data[5], packet.data[6], packet.data[7], packet.data[8], packet.data[9]);
    */

    decoded = avcodec_decode_audio3(aac_ctx, (int16_t *)dec_buf1, &data_size, &packet);
    //log_print(LOG_ERR, "AAC_decode_frame: decoded:%d, data_size:%d, frame_number:%d\n",decoded, data_size, aac_ctx->frame_number);
    
    if(decoded < 0){
        log_print(LOG_ERR,"avcodec_decode_audio3 error!\n");
        if(packet.size > 0){
            memcpy(rbuf, packet.data, packet.size);
            AAC_read(rbuf+packet.size, 4096-packet.size);
            packet.data = rbuf;
            packet.size = 4096;
        }
        return 0;
    }
    
    packet.data += decoded;
    packet.size -= decoded;

    if(data_size <= 0){
        return 0 ;
    }

    memcpy(buf, dec_buf1, data_size);

    return data_size;
}

am_codec_struct AAC_codec=
{
    .name="aac",
    .format=AUDIO_FORMAT_AAC,
    .used=0,
    .init=AAC_init,
    .release=AAC_release,
    .decode_frame=AAC_decode_frame,
};


int register_aac_codec(void)
{
    return register_audio_codec(&AAC_codec);
}

