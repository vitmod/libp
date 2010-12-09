#include "adec.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <syslog.h>

static AVPacket packet;
static uint8_t rbuf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];

static uint8_t dec_buf1[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
static uint8_t dec_buf2[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
static uint8_t * audio_buf = NULL;

static adec_feeder_t * wmapro_feeder = NULL;
static AVCodec * wmapro_codec = NULL;
static AVCodecContext * wmapro_ctx = NULL;

int WMAPro_init(adec_feeder_t *feeder, void * arg)
{

    if(feeder == NULL || arg == NULL){
        return -1;
    }

    wmapro_ctx =  (AVCodecContext*)arg;

    wmapro_feeder = feeder;
    wmapro_feeder->data_width = 16;

    wmapro_codec = avcodec_find_decoder(CODEC_ID_WMAPRO);
    if (!wmapro_codec){
        log_print(LOG_ERR, "avcodec_finde_decoder(CODEC_ID_WMAPRO) error!\n");
        return -1;
    }

    if (avcodec_open(wmapro_ctx, wmapro_codec) < 0){
        log_print(LOG_ERR, "avcodec_open() error!\n");
        return -1;
    }

    av_init_packet(&packet);
    packet.size = 0;
    packet.data = NULL;

    log_print(LOG_INFO, "[%s,%d]codec init ok!\n", __func__, __LINE__);
    return 0;
}

int WMAPro_release(void)
{
    avcodec_close(wmapro_ctx);
    return 0;
}


static inline int WMAPro_read(char *buf,int size)
{
    return wmapro_feeder->get_bytes(buf, size);
}

int WMAPro_decode_frame(char *buf,int len,struct frame_fmt *fmt)
{
    int decoded;
    int data_size;

    if(packet.size  <= 0){
        int ret = 0;
        packet.data = rbuf;
        packet.size = wmapro_ctx->block_align;
        ret = WMAPro_read(rbuf, packet.size);

        if(ret <=0){
            return 0;
        }
    }

    data_size = sizeof(dec_buf1);
    decoded = avcodec_decode_audio3(wmapro_ctx, (int16_t *)dec_buf1, &data_size, &packet);

    if(decoded < 0){
        log_print(LOG_ERR,"avcodec_decode_audio3 error!\n");
        packet.size = 0;
    }

    packet.data += decoded;
    packet.size -= decoded;

    int sample_size = av_get_bits_per_sample_format(wmapro_ctx->sample_fmt)/8;
    int samples = data_size / sample_size;
    int samples_per_channel = samples / wmapro_ctx->channels;
    int i;
    uint8_t * po;
    uint8_t * pi;
    po = dec_buf2;
    pi = dec_buf1;
    for(i = 0; i < samples_per_channel; i++){
        //*(uint16_t *)po = ((int)rint(*(const float *)pi * (1<<15)) + (int)rint(*(const float *)(pi +4) * (1<<15)))/2;
        *(uint16_t *)po = (int)rint(*(const float *)pi * (1<<15));
        po += 2;
        //*(uint16_t *)po = ((int)rint(*(const float *)(pi + sample_size)* (1<<15)) + (int)rint(*(const float *)(pi +4) * (1<<15)))/2;
        *(uint16_t *)po = (int)rint(*(const float *)(pi + sample_size)* (1<<15)); 
        pi += sample_size * wmapro_ctx->channels;
        po += 2;
    }

    data_size = samples_per_channel * 2 * 2;

    memcpy(buf, dec_buf2, data_size);

    return data_size;
}

am_codec_struct WMAPro_codec=
{
    .name="wmapro",
    .format=AUDIO_FORMAT_WMAPRO,
    .used=0,
    .init=WMAPro_init,
    .release=WMAPro_release,
    .decode_frame=WMAPro_decode_frame,
};


int register_wmapro_codec(void)
{
    return register_audio_codec(&WMAPro_codec);
}

