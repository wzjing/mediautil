#include "video_filter.h"
#include "log.h"

#define TAG "video_filter"

int VideoFilter::create(const char *filter_descr, VideoConfig *inConfig, VideoConfig *outConfig) {
    this->description = filter_descr;
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {outConfig->format, AV_PIX_FMT_NONE};

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             inConfig->width, inConfig->height, inConfig->format,
             inConfig->timebase.num, inConfig->timebase.den,
             inConfig->pixel_aspect.num, inConfig->pixel_aspect.num);
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, nullptr, filter_graph);
    if (ret < 0) {
        LOGE(TAG, "Cannot create buffer source\n");
        goto end;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       nullptr, nullptr, filter_graph);
    if (ret < 0) {
        LOGE(TAG, "Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE(TAG, "Cannot set output pixel format\n");
        goto end;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr,
                                        &inputs, &outputs, nullptr)) < 0) {
        LOGD(TAG, "Error: unable to parse filter description:%s\n\t%s\n", filter_descr, av_err2str(ret));
        goto end;
    }

    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0) {
        LOGD(TAG, "Error: unable to configure filter graph:\n\t%s\n", av_err2str(ret));
        goto end;
    }

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int VideoFilter::filter(AVFrame *source, AVFrame *dest) {

    int ret = av_buffersrc_add_frame_flags(buffersrc_ctx, source, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        LOGE(TAG, "unable to add frame to filter chain\n");
        return -1;
    }
    ret = av_buffersink_get_frame(buffersink_ctx, dest);
    if (ret < 0) {
        LOGE(TAG, "unable to get frame from filter chain\n");
        return -1;
    }
    return 0;
}

void VideoFilter::dumpGraph() {
    LOGD(TAG, "Video Filer(%s):\n%s\n", this->description, avfilter_graph_dump(filter_graph, nullptr));
}

void VideoFilter::destroy() {
    if (filter_graph)
        avfilter_graph_free(&filter_graph);
}
