#ifndef MEDIA_ASF_MEDIASOURCE_H_
#define MEDIA_ASF_MEDIASOURCE_H_

#include  "MediaSource.h"
#include  "DataSource.h"
#include  "MediaBufferGroup.h"
#include  "MetaData.h"
#include  "audio_mediasource.h"
#include  "../audio-dec.h"

namespace android {

typedef int (*fp_read_buffer)(unsigned char *,int);

//should corespond to  the CodecID defined in  <amffmpeg/libavcodec/avcodec.h>
#define CODEC_ID_WMAV1_FFMPEG  0x15007    
#define CODEC_ID_WMAV2_FFMPEG  0x15008 //wma
#define CODEC_ID_WMAPRO_FFMPEG 0x15028 //wmapro

#define CODEC_ID_WMAV1_OMX    0x160
#define CODEC_ID_WMAV2_OMX    0x161
#define CODEC_ID_WMAPRO_OMX   




class Asf_MediaSource : public AudioMediaSource 
{
public:
	Asf_MediaSource(void *read_buffer,aml_audio_dec_t *audec);
	
    status_t start(MetaData *params = NULL);
    status_t stop();
    sp<MetaData> getFormat();
    status_t read(MediaBuffer **buffer, const ReadOptions *options = NULL);
	
	int  GetReadedBytes();
	int GetSampleRate();
	int GetChNum();
	int* Get_pStop_ReadBuf_Flag();
	int Set_pStop_ReadBuf_Flag(int *pStop);

	int set_Asf_MetaData(aml_audio_dec_t *audec);
	int MediaSourceRead_buffer(unsigned char *buffer,int size);
		
	fp_read_buffer fpread_buffer;
	
	int sample_rate;
	int ChNum;
	int frame_size;
	int *pStop_ReadBuf_Flag;
	int extradata_size;
	int64_t bytes_readed_sum_pre;
	int64_t bytes_readed_sum;
protected:
    virtual ~Asf_MediaSource();

private:
	bool mStarted;
    sp<DataSource> mDataSource;
    sp<MetaData>   mMeta;
    MediaBufferGroup *mGroup;	
    int64_t mCurrentTimeUs;	
	int     mBytesReaded;
	int block_align;
    Asf_MediaSource(const Asf_MediaSource &);
    Asf_MediaSource &operator=(const Asf_MediaSource &);
};


}

#endif

