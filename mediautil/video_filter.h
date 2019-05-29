
#ifndef VIDEOBOX_VIDEO_FILTER_H
#define VIDEOBOX_VIDEO_FILTER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

struct VideoConfig {
    AVPixelFormat format;
    int width;
    int height;
    AVRational timebase{1, 30};
    AVRational pixel_aspect{1, 1};

    VideoConfig(AVPixelFormat format, int width, int height, AVRational timebase = {1, 30},
                AVRational pixel_aspect = {1, 1}) {
        this->format = format;
        this->width = width;
        this->height = height;
    }
};

class VideoFilter {
protected:
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
    const char *description = nullptr;
public:

    VideoFilter() = default;

    int create(const char *filter_descr, VideoConfig *inConfig, VideoConfig *outConfig);

    int filter(AVFrame *source, AVFrame *dest);

    void dumpGraph();

    void destroy();
};


#endif //VIDEOBOX_VIDEO_FILTER_H
