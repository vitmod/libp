
#define LOG_TAG "amavutils"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <cutils/log.h>
#include <sys/ioctl.h>
#include "include/Amvideoutils.h"
#include "include/Amsysfsutils.h"


#define SYSCMD_BUFSIZE 40
#define DISP_DEVICE_PATH "/sys/class/video/device_resolution"
#define FB_DEVICE_PATH   "/sys/class/graphics/fb0/virtual_size"
#define ANGLE_PATH       "/sys/class/ppmgr/angle"
#define VIDEO_GLOBAL_OFFSET_PATH "/sys/class/video/global_offset"

static int rotation = 0;
static int disp_width = 1920;
static int disp_height = 1080;

//#define LOG_FUNCTION_NAME LOGI("%s-%d\n",__FUNCTION__,__LINE__);
#define LOG_FUNCTION_NAME


int  amvideo_utils_get_global_offset(void)
{
    LOG_FUNCTION_NAME
    int offset = 0;
    char buf[SYSCMD_BUFSIZE];
    int ret;
    ret = amsysfs_get_sysfs_str(VIDEO_GLOBAL_OFFSET_PATH, buf, SYSCMD_BUFSIZE);
    if (ret < 0) {
        return offset;
    }
    if (sscanf(buf, "%d", &offset) == 1) {
        LOGI("video global_offset %d\n", offset);
    }
    return offset;
}

static inline int amvideo_utils_open(int id)
{
    LOG_FUNCTION_NAME

    return open("/sys/class/video/axis", O_RDWR);
}

int amvideo_utils_set_virtual_position(int32_t x, int32_t y, int32_t w, int32_t h, int rotation)
{
    LOG_FUNCTION_NAME
    int fd;
    int dev_fd, dev_w, dev_h, disp_w, disp_h, video_global_offset;
    int dst_x, dst_y, dst_w, dst_h;
    char buf[SYSCMD_BUFSIZE];
    int angle_fd = -1;
    int ret = -1;

    LOGI("amvideo_utils_set_virtual_position:: x=%d y=%d w=%d h=%d\n", x, y, w, h);

    bzero(buf, SYSCMD_BUFSIZE);

    dst_x = x;
    dst_y = y;
    dst_w = w;
    dst_h = h;

    fd = amvideo_utils_open(1);
    if (fd < 0) {
        goto OUT;
    }

    dev_fd = open(DISP_DEVICE_PATH, O_RDONLY);
    if (dev_fd < 0) {
        goto OUT;
    }

    read(dev_fd, buf, SYSCMD_BUFSIZE);

    if (sscanf(buf, "%dx%d", &dev_w, &dev_h) == 2) {
        LOGI("device resolution %dx%d\n", dev_w, dev_h);
    } else {
        close(dev_fd);
        ret = -2;
        goto OUT;
    }
    close(dev_fd);

    amdisplay_utils_get_size(&disp_w, &disp_h);
    video_global_offset = amvideo_utils_get_global_offset();

    if (((disp_w != dev_w) || (disp_h / 2 != dev_h)) && (video_global_offset == 0)) {
        dst_x = dst_x * dev_w / disp_w;
        dst_y = dst_y * dev_h / disp_h;
        dst_w = dst_w * dev_w / disp_w;
        dst_h = dst_h * dev_h / disp_h;
    }

    angle_fd = open(ANGLE_PATH, O_WRONLY);
    if (angle_fd >= 0) {
        const char *angle[4] = {"0", "1", "2", "3"};
        write(angle_fd, angle[(rotation/90) & 3], 2);

        LOGI("set ppmgr angle %s\n", angle[(rotation/90) & 3]);
    }

    if (((rotation == 90) || (rotation == 270)) && (angle_fd < 0)) {
        if (dst_h == disp_h) {
            int center = x + w / 2;

            if (abs(center - disp_w / 2) < 2) {
                /* a centered overlay with rotation, change to full screen */
                dst_x = 0;
                dst_y = 0;
                dst_w = dev_w;
                dst_h = dev_h;

                LOGI("centered overlay expansion");
            }
        }
    }

    if (angle_fd >= 0) {
        close(angle_fd);
    }
    ret = 0;
OUT:
    snprintf(buf, 40, "%d %d %d %d", dst_x, dst_y, dst_x + dst_w - 1, dst_y + dst_h - 1);
    write(fd, buf, strlen(buf));

    LOGI("amvideo_utils_set_virtual_position (corrected):: x=%d y=%d w=%d h=%d\n", dst_x, dst_y, dst_w, dst_h);

    return ret;
}

int amvideo_utils_set_absolute_position(int32_t x, int32_t y, int32_t w, int32_t h, int rotation)
{
    LOG_FUNCTION_NAME
    int fd;
    int dev_fd, dev_w, dev_h, video_global_offset;
    int dst_x, dst_y, dst_w, dst_h;
    char buf[SYSCMD_BUFSIZE];
    int angle_fd = -1;
    int ret = -1;

    LOGI("amvideo_utils_set_absolute_position:: x=%d y=%d w=%d h=%d\n", x, y, w, h);

    dst_x = x;
    dst_y = y;
    dst_w = w;
    dst_h = h;

    fd = amvideo_utils_open(1);
    if (fd < 0) {
        goto OUT;
    }

    angle_fd = open(ANGLE_PATH, O_WRONLY);
    if (angle_fd >= 0) {
        const char *angle[4] = {"0", "1", "2", "3"};
        write(angle_fd, angle[(rotation/90) & 3], 2);

        LOGI("set ppmgr angle %s\n", angle[(rotation/90) & 3]);
    }

    if (angle_fd >= 0) {
        close(angle_fd);
    }
    ret = 0;
OUT:
    snprintf(buf, 40, "%d %d %d %d", dst_x, dst_y, dst_x + dst_w - 1, dst_y + dst_h - 1);
    write(fd, buf, strlen(buf));

    LOGI("amvideo_utils_set_absolute_position (corrected):: x=%d y=%d w=%d h=%d\n", dst_x, dst_y, dst_w, dst_h);

    return ret;
}



int amvideo_utils_get_position(int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
    LOG_FUNCTION_NAME
    int ret = -1;
    char buf[SYSCMD_BUFSIZE];
    int x1, y1;
    int fd;

    fd = amvideo_utils_open(1);
    if (fd < 0) {
        goto OUT;
    }

    read(fd, buf, SYSCMD_BUFSIZE);

    sscanf(buf, "%d %d %d %d", x, y, &x1, &y1);

    *w = x1 - *x + 1;
    *h = y1 - *y + 1;
    ret = 0;
OUT:
    return 0;
}


