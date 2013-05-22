/*
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
*/
//#include  "../../../../../frameworks/native/include/media/openmax/OMX_Core.h"
//#include  "../../../../../frameworks/native/include/media/openmax/OMX_Index.h"
#include "OMX_Index.h"
#include "OMX_Core.h"
#include "OMXClient.h"
#include "audio_medissource.h"
#include "StrongPointer.h"
namespace android 
{

	
class AmlOMXCodec
{
public:
	AmlOMXCodec(int codec_type,void *read_buffer,int *exit);
	OMXClient 			  m_OMXClient;		
	sp<AudioMediaSource>  m_OMXMediaSource;
	int                   read(unsigned char *buf,unsigned *size,int *exit);
	virtual status_t      start();
	void                  pause();
	void                  stop();
	int                   GetDecBytes();
	int                   started_flag;
//protected:
	virtual ~AmlOMXCodec();
private:	
	sp<MediaSource>	      m_codec;
};



}