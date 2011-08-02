#include <log_print.h>
void print_version_info()
{
#ifdef HAVE_VERSION_INFO

#ifdef LIBPLAYER_GIT_VERSION
log_print("LibPlayer git version:%s\n",LIBPLAYER_GIT_VERSION);
#endif
#ifdef LIBPLAYER_LAST_CHANGED
log_print("LibPlayer Last Changed:%s\n",LIBPLAYER_LAST_CHANGED);
#endif
#ifdef LIBPLAYER_BUILD_TIME
log_print("LibPlayer Last Build:%s\n",LIBPLAYER_BUILD_TIME);
#endif

#ifdef LIBPLAYER_BUILD_NAME
log_print("LibPlayer Builer Name:%s\n",LIBPLAYER_BUILD_NAME);
#endif


#endif
}

