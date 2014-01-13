#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "amlv4l.h"
#include "amvdec_priv.h"
#define V4LDEVICE_NAME  "/dev/video13"
#define CLEAR(s) memset(&s, 0, sizeof(s))

static int amlv4l_unmapbufs(amvideo_dev_t *dev);
static int amlv4l_mapbufs(amvideo_dev_t *dev);
int amlv4l_setfmt(amvideo_dev_t *dev, struct v4l2_format *fmt);
int amlv4l_init(amvideo_dev_t *dev, int type, int width, int height, int fmt, int buffernum) {
    int ret;
    amlv4l_dev_t *v4l = dev->devpriv;
    struct v4l2_format v4lfmt;
    ret = open(V4LDEVICE_NAME, O_RDWR | O_NONBLOCK);
    if (ret < 0) {
        LOGE("v4l device opend failed!,ret=%d,%s(%d)\n", ret, strerror(errno), errno);
        return errno;
    }
    v4l->v4l_fd = ret;
    v4l->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l->width = width;
    v4l->height = height;
    v4l->pixformat = fmt;
    v4l->buffer_num = buffernum;
    v4lfmt.type = v4l->type;
    v4lfmt.fmt.pix.width = v4l->width;
    v4lfmt.fmt.pix.height = v4l->height;
    v4lfmt.fmt.pix.pixelformat = v4l->pixformat;
    ret = amlv4l_setfmt(dev, &v4lfmt);
    if (ret != 0) {
        goto error_out;
    }
    ret = amlv4l_mapbufs(dev);
error_out:
    return ret;
}

static int amlv4l_ioctl(amvideo_dev_t *dev, int request, void *arg) {
    int ret;
    amlv4l_dev_t *v4l = dev->devpriv;
    ret = ioctl(v4l->v4l_fd, request, arg);
    if (ret == -1 && errno) {
        ("amlv4l_ioctlfailed!,request=%x,ret=%d,%s(%d)\n", request, ret, strerror(errno), errno);
        ret = -errno;
    }
    return ret;
}

int amlv4l_release(amvideo_dev_t *dev) {
    int ret = -1;
    amlv4l_dev_t *v4l = dev->devpriv;
    if (v4l->v4l_fd < 0) {
        return 0;
    }
    amlv4l_stop(dev);
    amlv4l_unmapbufs(dev);
    if (v4l->v4l_fd >= 0) {
        ret = close(v4l->v4l_fd);
    }
    v4l->v4l_fd = -1;
    free(dev);
    if (ret == -1 && errno) {
        ret = -errno;
    }

    return ret;
}

int amlv4l_dequeue_buf(amvideo_dev_t *dev, vframebuf_t *vf) {
    struct v4l2_buffer vbuf;
    CLEAR(vbuf);
    int ret;
    amlv4l_dev_t *v4l = dev->devpriv;
    vbuf.type = v4l->type;
    vbuf.memory = v4l->memory_mode;
    vbuf.length = vf->length;
    ret = amlv4l_ioctl(dev, VIDIOC_DQBUF, &vbuf);
    if (!ret && vbuf.index < v4l->buffer_num) {
        vf->pts = vbuf.timestamp.tv_sec & 0xFFFFFFFF;
        vf->pts <<= 32;
        vf->pts += vbuf.timestamp.tv_usec & 0xFFFFFFFF;
        vf->fd = vbuf.m.fd;
        vf->index = vbuf.index;
    }

    return ret;
}

int amlv4l_queue_buf(amvideo_dev_t *dev, vframebuf_t *vf) {
    struct v4l2_buffer vbuf;
    CLEAR(vbuf);
    int ret;
    amlv4l_dev_t *v4l = dev->devpriv;
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_DMABUF;
    vbuf.index = vf->index;
    vbuf.m.fd = vf->fd;
    vbuf.length = vf->length;
    return amlv4l_ioctl(dev, VIDIOC_QBUF, &vbuf);
}

int amlv4l_start(amvideo_dev_t *dev) {
    int type;
    amlv4l_dev_t *v4l = dev->devpriv;
    type = v4l->type;
    return amlv4l_ioctl(dev, VIDIOC_STREAMON, &type);
}

int amlv4l_stop(amvideo_dev_t *dev) {
    int type;
    amlv4l_dev_t *v4l = dev->devpriv;
    type = v4l->type;
    return amlv4l_ioctl(dev, VIDIOC_STREAMOFF, &type);
}

int amlv4l_setfmt(amvideo_dev_t *dev, struct v4l2_format *fmt) {
    int ret = amlv4l_ioctl(dev, VIDIOC_S_FMT, fmt);
    if (ret != 0) {
        LOGE("VIDIOC_G_FMT failed,ret=%d\n", ret);
        return ret;
    }
    return amlv4l_ioctl(dev, VIDIOC_G_FMT, fmt);
}

static int amlv4l_unmapbufs(amvideo_dev_t *dev) {
    return 0;
}

static int amlv4l_mapbufs(amvideo_dev_t *dev) {
    int ret;
    struct v4l2_requestbuffers rb;
    CLEAR(rb);
    amlv4l_dev_t *v4l = dev->devpriv;
    rb.count = v4l->buffer_num;
    rb.type = v4l->type;
    rb.memory = v4l->memory_mode;
    ret = amlv4l_ioctl(dev, VIDIOC_REQBUFS, &rb);
    if (ret != 0) {
        LOGE("VIDIOC_REQBUFS failed,ret=%d\n", ret);
    }
    return ret;
}

amvideo_dev_t *new_amlv4l(void) {
    amvideo_dev_t *dev;
    amlv4l_dev_t *v4l;
    dev = malloc(sizeof(amvideo_dev_t) + sizeof(amlv4l_dev_t));
    memset(dev, 0, sizeof(amvideo_dev_t) + sizeof(amlv4l_dev_t));
    dev->devpriv = (void *) ((int) (&dev->devpriv) + 4);
    v4l = dev->devpriv;
    v4l->memory_mode = V4L2_MEMORY_DMABUF;
    dev->ops.init = amlv4l_init;
    dev->ops.release = amlv4l_release;
    dev->ops.dequeuebuf = amlv4l_dequeue_buf;
    dev->ops.queuebuf = amlv4l_queue_buf;
    dev->ops.start = amlv4l_start;
    dev->ops.stop = amlv4l_stop;

    return dev;
}

