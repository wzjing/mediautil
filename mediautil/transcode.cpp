//
// Created by android1 on 2019/5/29.
//

#include "transcode.h"
#include "log.h"

#define TAG "transcode"

#define check(ret, msg) if(ret < 0){LOGD(TAG, "%s: %s\n", msg, av_err2str(ret));return -1;}

int transcode(const char *output_filename, const char *input_filename, Config *new_config) {
    AVFormatContext *inFmtCtx = nullptr;
    AVFormatContext *outFmtCtx = nullptr;

    AVCodecContext *vDecCtx = nullptr;
    AVCodecContext *aDecCtx = nullptr;
    AVCodecContext *vEncCtx = nullptr;
    AVCodecContext *aEncCtx = nullptr;

    AVStream *vInStream = nullptr;
    AVStream *vOutStream = nullptr;
    AVStream *aInStream = nullptr;
    AVStream *aOutStream = nullptr;

    int ret;

    // open input file
    ret = avformat_open_input(&inFmtCtx, input_filename, nullptr, nullptr);
    check(ret, "unable to open input file")
    ret = avformat_find_stream_info(inFmtCtx, nullptr);
    check(ret, "unable to get input file info")

    for (int i = 0; i < inFmtCtx->nb_streams; ++i) {
        AVStream *inStream = inFmtCtx->streams[i];
        if (inStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AVCodec *decoder = avcodec_find_decoder(inStream->codecpar->codec_id);
            aDecCtx = avcodec_alloc_context3(decoder);
            ret = avcodec_parameters_to_context(vDecCtx, inStream->codecpar);
            check(ret, "unable to initial audio decode context")
            ret = avcodec_open2(aDecCtx, decoder, nullptr);
            check(ret, "unable to open audio decode context")

            AVCodec* encoder = avcodec_find_encoder(new_config->audio_codec_id);
            AVStream* outStream = avformat_new_stream(outFmtCtx, encoder);
            if (!outStream) {
                LOGE(TAG, "unable to create out audio stream\n");
                return -1;
            }
        } else if (inStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {

        }
    }

    return 0;
}
