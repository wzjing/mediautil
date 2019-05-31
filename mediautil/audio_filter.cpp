//
// Created by android1 on 2019/5/22.
//

#include "audio_filter.h"
#include "log.h"

#define TAG "audio_filter"

int AudioFilter::create(const char *filter_descr, AudioConfig *inConfig1,
                        AudioConfig *inConfig2, AudioConfig *outConfig) {
    this->description = filter_descr;
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("abuffer");
    const AVFilter *buffersink = avfilter_get_by_name("abuffersink");
    AVFilterInOut *output = avfilter_inout_alloc();
    AVFilterInOut *inputs[2];
    inputs[0] = avfilter_inout_alloc();
    inputs[1] = avfilter_inout_alloc();

    char ch_layout[128];
    int nb_channels = 0;
    int pix_fmts[] = {outConfig->format, AV_SAMPLE_FMT_NONE};

    filter_graph = avfilter_graph_alloc();
    if (!inputs[0] || !inputs[1] || !output || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }


    nb_channels = av_get_channel_layout_nb_channels(inConfig1->ch_layout);
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), nb_channels, inConfig1->ch_layout);
    snprintf(args, sizeof(args),
             "sample_rate=%d:sample_fmt=%d:channel_layout=%s:channels=%d:time_base=%d/%d",
             inConfig1->sample_rate,
             inConfig1->format,
             ch_layout,
             nb_channels,
             inConfig1->timebase.num,
             inConfig1->timebase.den);
    ret = avfilter_graph_create_filter(&buffersrc1_ctx, buffersrc, "in1",
                                       args, nullptr, filter_graph);
    if (ret < 0) {
        LOGE(TAG, "Cannot create buffer source\n");
        goto end;
    }

    nb_channels = av_get_channel_layout_nb_channels(inConfig2->ch_layout);
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), nb_channels, inConfig2->ch_layout);
    snprintf(args, sizeof(args),
             "sample_rate=%d:sample_fmt=%d:channel_layout=%s:channels=%d:time_base=%d/%d",
             inConfig2->sample_rate,
             inConfig2->format,
             ch_layout,
             nb_channels,
             inConfig2->timebase.num,
             inConfig2->timebase.den);
    ret = avfilter_graph_create_filter(&buffersrc2_ctx, buffersrc, "in2",
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

    ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", pix_fmts,
                              AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE(TAG, "Cannot set output sample format\n");
        goto end;
    }

    inputs[0]->name = av_strdup("in1");
    inputs[0]->filter_ctx = buffersrc1_ctx;
    inputs[0]->pad_idx = 0;
    inputs[0]->next = inputs[1];

    inputs[1]->name = av_strdup("in2");
    inputs[1]->filter_ctx = buffersrc2_ctx;
    inputs[1]->pad_idx = 0;
    inputs[1]->next = nullptr;

    output->name = av_strdup("out");
    output->filter_ctx = buffersink_ctx;
    output->pad_idx = 0;
    output->next = nullptr;

    avfilter_graph_set_auto_convert(filter_graph, AVFILTER_AUTO_CONVERT_NONE);
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr,
                                        &output, inputs, nullptr)) < 0) {
        LOGD(TAG, "Error: unable to parse filter description:%s\n\t%s\n", filter_descr, av_err2str(ret));
        goto end;
    }

    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0) {
        LOGD(TAG, "Error: unable to configure filter graph:\n\t%s\n", av_err2str(ret));
        goto end;
    }

    end:
    avfilter_inout_free(inputs);
    avfilter_inout_free(&output);

    return ret;
}

int AudioFilter::create(const char *filter_descr, AudioConfig *inConfig, AudioConfig *outConfig) {
    this->description = filter_descr;
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("abuffer");
    const AVFilter *buffersink = avfilter_get_by_name("abuffersink");
    AVFilterInOut *output = avfilter_inout_alloc();
    AVFilterInOut *input = avfilter_inout_alloc();

    char ch_layout[128];
    int nb_channels = 0;
    int pix_fmts[] = {outConfig->format, AV_SAMPLE_FMT_NONE};

    filter_graph = avfilter_graph_alloc();
    if (!input || !output || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }


    nb_channels = av_get_channel_layout_nb_channels(inConfig->ch_layout);
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), nb_channels, inConfig->ch_layout);
    snprintf(args, sizeof(args),
             "sample_rate=%d:sample_fmt=%d:channel_layout=%s:channels=%d:time_base=%d/%d",
             inConfig->sample_rate,
             inConfig->format,
             ch_layout,
             nb_channels,
             inConfig->timebase.num,
             inConfig->timebase.den);
    ret = avfilter_graph_create_filter(&buffersrc1_ctx, buffersrc, "in1",
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

    ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", pix_fmts,
                              AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE(TAG, "Cannot set output sample format\n");
        goto end;
    }

    input->name = av_strdup("in");
    input->filter_ctx = buffersrc1_ctx;
    input->pad_idx = 0;
    input->next = nullptr;

    output->name = av_strdup("out");
    output->filter_ctx = buffersink_ctx;
    output->pad_idx = 0;
    output->next = nullptr;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr,
                                        &output, &input, nullptr)) < 0) {
        LOGD(TAG, "Error: unable to parse filter description:%s\n\t%s\n", filter_descr, av_err2str(ret));
        goto end;
    }

    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0) {
        LOGD(TAG, "Error: unable to configure filter graph:\n\t%s\n", av_err2str(ret));
        goto end;
    }

    end:
    avfilter_inout_free(&input);
    avfilter_inout_free(&output);

    return ret;
}

void AudioFilter::dumpGraph() {
    LOGI(TAG, "AudioFilter Graph(%s):\n%s\n", this->description, avfilter_graph_dump(filter_graph, nullptr));
}

void AudioFilter::destroy() {
    if (filter_graph)
        avfilter_graph_free(&filter_graph);
}

int AudioFilter::filter(AVFrame *input1, AVFrame *input2, AVFrame *result) {
    int ret = av_buffersrc_add_frame_flags(buffersrc1_ctx, input1, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        LOGE(TAG, "add audio input1 error: %s\n", av_err2str(ret));
        return ret;
    }

    ret = av_buffersrc_add_frame_flags(buffersrc2_ctx, input2, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        LOGE(TAG, "add audio input1 error: %s\n", av_err2str(ret));
        return ret;
    }

    return av_buffersink_get_samples(buffersink_ctx, result, result->nb_samples);
}

int AudioFilter::getFrame(AVFrame *result) {
    int ret = av_buffersink_get_samples(buffersink_ctx, result, result->nb_samples);
    return ret;
}

int AudioFilter::addInput1(AVFrame *input) {
    return av_buffersrc_add_frame_flags(buffersrc1_ctx, input, AV_BUFFERSRC_FLAG_KEEP_REF);
}

int AudioFilter::addInput2(AVFrame *input) {
    return av_buffersrc_add_frame_flags(buffersrc2_ctx, input, AV_BUFFERSRC_FLAG_KEEP_REF);
}
