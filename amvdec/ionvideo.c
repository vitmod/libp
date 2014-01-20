#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ionvideo.h>
#include "ionvdec_priv.h"
#include "ionv4l.h"

ionvideo_dev_t *new_ionvideo(int flags) {
    ionvideo_dev_t *dev = NULL;
    if (flags & FLAGS_V4L_MODE) {
        dev = new_ionv4l();
        if (!dev) {
            LOGE("alloc v4l devices failed.\n");
        } else {
            dev->mode = FLAGS_V4L_MODE;
        }
    }
    return dev;
}
int ionvideo_setparameters(ionvideo_dev_t *dev, int cmd, void * parameters) {
    return 0;
}
int ionvideo_init(ionvideo_dev_t *dev, int flags, int width, int height, int fmt, int buffernum) {
    int ret = -1;
    if (dev->ops.init) {
        ret = dev->ops.init(dev, O_RDWR | O_NONBLOCK, width, height, fmt, buffernum);
        LOGE("ionvideo_init ret=%d\n", ret);
    }
    return ret;
}
int ionvideo_start(ionvideo_dev_t *dev) {
    if (dev->ops.start) {
        return dev->ops.start(dev);
    }
    return 0;
}
int ionvideo_stop(ionvideo_dev_t *dev) {
    if (dev->ops.stop) {
        return dev->ops.stop(dev);
    }
    return 0;
}
int ionvideo_release(ionvideo_dev_t *dev) {
    if (dev->mode == FLAGS_V4L_MODE) {
        ionv4l_release(dev);
    }
    return 0;
}
int ionv4l_dequeuebuf(ionvideo_dev_t *dev, vframebuf_t*vf) {
    if (dev->ops.dequeuebuf) {
        return dev->ops.dequeuebuf(dev, vf);
    }
    return -1;
}
int ionv4l_queuebuf(ionvideo_dev_t *dev, vframebuf_t*vf) {
    if (dev->ops.queuebuf) {
        return dev->ops.queuebuf(dev, vf);
    }
    return 0;
}
