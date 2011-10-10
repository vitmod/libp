#ifndef PLAYER_THUMBNAIL_H
#define PLAYER_THUMBNAIL_H

#ifdef  __cplusplus
extern "C" {
#endif

void * register_decoder(void);
int decoder_init(void *handle, const char* filename);
int decoder_start(void *handle);
int decoder_read(void *handle, char* buffer);
void get_video_size(void *handle, int* width, int* height);
int decoder_release(void *handle);
void unregister_decoder(void* handle);

#ifdef  __cplusplus
}
#endif

#endif
