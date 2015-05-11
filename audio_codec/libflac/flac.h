/*
 * FLAC (Free Lossless Audio Codec) decoder/demuxer common functions
 * Copyright (c) 2008 Justin Ruggles
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/flac.h
 * FLAC (Free Lossless Audio Codec) decoder/demuxer common functions
 */

#ifndef AVCODEC_FLAC_H
#define AVCODEC_FLAC_H

#include "avcodec.h"

#define FLAC_STREAMINFO_SIZE   34
#define FLAC_MAX_CHANNELS       8
#define FLAC_MIN_BLOCKSIZE     16
#define FLAC_MAX_BLOCKSIZE  65535
#define FLAC_MIN_FRAME_SIZE    11

enum {
    FLAC_CHMODE_INDEPENDENT =  0,
    FLAC_CHMODE_LEFT_SIDE   =  8,
    FLAC_CHMODE_RIGHT_SIDE  =  9,
    FLAC_CHMODE_MID_SIDE    = 10,
};

enum {
    FLAC_METADATA_TYPE_STREAMINFO = 0,
    FLAC_METADATA_TYPE_PADDING,
    FLAC_METADATA_TYPE_APPLICATION,
    FLAC_METADATA_TYPE_SEEKTABLE,
    FLAC_METADATA_TYPE_VORBIS_COMMENT,
    FLAC_METADATA_TYPE_CUESHEET,
    FLAC_METADATA_TYPE_PICTURE,
    FLAC_METADATA_TYPE_INVALID = 127
};

enum FLACExtradataFormat {
    FLAC_EXTRADATA_FORMAT_STREAMINFO  = 0,
    FLAC_EXTRADATA_FORMAT_FULL_HEADER = 1
};

#define FLACCOMMONINFO \
    int samplerate;         /**< sample rate                             */\
    int channels;           /**< number of channels                      */\
    int bps;                /**< bits-per-sample                         */\

/**
 * Data needed from the Streaminfo header for use by the raw FLAC demuxer
 * and/or the FLAC decoder.
 */
#define FLACSTREAMINFO \
    FLACCOMMONINFO \
    int max_blocksize;      /**< maximum block size, in samples          */\
    int max_framesize;      /**< maximum frame size, in bytes            */\
    int64_t samples;        /**< total number of samples                 */\

typedef struct FLACStreaminfo {
    FLACSTREAMINFO
} FLACStreaminfo;

typedef struct FLACFrameInfo {
    FLACCOMMONINFO
    int blocksize;          /**< block size of the frame                 */
    int ch_mode;            /**< channel decorrelation mode              */
} FLACFrameInfo;

/**
 * Parse the Streaminfo metadata block
 * @param[out] avctx   codec context to set basic stream parameters
 * @param[out] s       where parsed information is stored
 * @param[in]  buffer  pointer to start of 34-byte streaminfo data
 */
void ff_flac_parse_streaminfo(AVCodecContext *avctx, struct FLACStreaminfo *s,
                              const uint8_t *buffer);

/**
 * Validate the FLAC extradata.
 * @param[in]  avctx codec context containing the extradata.
 * @param[out] format extradata format.
 * @param[out] streaminfo_start pointer to start of 34-byte STREAMINFO data.
 * @return 1 if valid, 0 if not valid.
 */
int ff_flac_is_extradata_valid(AVCodecContext *avctx,
                               enum FLACExtradataFormat *format,
                               uint8_t **streaminfo_start);

/**
 * Parse the metadata block parameters from the header.
 * @param[in]  block_header header data, at least 4 bytes
 * @param[out] last indicator for last metadata block
 * @param[out] type metadata block type
 * @param[out] size metadata block size
 */
void ff_flac_parse_block_header(const uint8_t *block_header,
                                int *last, int *type, int *size);

/**
 * Calculate an estimate for the maximum frame size based on verbatim mode.
 * @param blocksize block size, in samples
 * @param ch number of channels
 * @param bps bits-per-sample
 */
int ff_flac_get_max_frame_size(int blocksize, int ch, int bps);

#endif /* AVCODEC_FLAC_H */
