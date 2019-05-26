//
// Created by android1 on 2019/4/22.
//

#ifndef VIDEOBOX_LOG_H
#define VIDEOBOX_LOG_H

#include <cstdio>

#ifdef DEBUG
#define LOGD(TAG, format, ...)  printf(TAG ":" format, ## __VA_ARGS__)
#define LOGI(TAG, format, ...) printf("\033[34m" TAG ":" format "\033[0m", ## __VA_ARGS__)
#else
#define LOGD(TAG, format, ...)
#define LOGI(TAG, format, ...)
#endif
#define LOGW(TAG, format, ...) printf("\033[33m" TAG ":" format "\033[0m", ## __VA_ARGS__)
#define LOGE(TAG, format, ...) fprintf(stderr,"\033[31m" TAG ":" format "\033[0m", ## __VA_ARGS__)

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
}

void logPacket(AVPacket *packet, AVRational *timebase, const char *tag);

void logFrame(AVFrame *frame, AVRational *timebase, const char *tag, int isVideo);

void logContext(AVCodecContext *context, const char *tag, int isVideo);

void logStream(AVStream *stream, const char *tag, int isVideo);

void logMetadata(AVDictionary *metadata, const char *tag);

#endif //VIDEOBOX_LOG_H
