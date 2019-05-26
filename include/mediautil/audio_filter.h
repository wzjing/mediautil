//
// Created by android1 on 2019/5/22.
//

#ifndef VIDEOBOX_AUDIO_FILTER_H
#define VIDEOBOX_AUDIO_FILTER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

struct AudioConfig {
    AVSampleFormat format = AV_SAMPLE_FMT_NONE;
    int sample_rate = 0;
    uint64_t ch_layout = AV_CH_LAYOUT_STEREO;
    AVRational timebase = (AVRational) {1, 1};
    AudioConfig(AVSampleFormat format, int sample_rate, uint64_t ch_layout, AVRational timebase) {
        this->format = format;
        this->sample_rate = sample_rate;
        this->ch_layout = ch_layout;
        this->timebase = timebase;
    }

};

class AudioFilter {
protected:
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc1_ctx;
    AVFilterContext *buffersrc2_ctx;
    AVFilterGraph *filter_graph;
    const char *description = nullptr;
public:
    AudioFilter() = default;

    int create(const char *filter_descr, AudioConfig* inConfig1, AudioConfig* inConfig2, AudioConfig* outConfig);

    void dumpGraph();

    int filter(AVFrame *input1, AVFrame* input2);

    int filter(AVFrame *input1, AVFrame* input2, AVFrame* result);

    void destroy();
};


#endif //VIDEOBOX_AUDIO_FILTER_H
