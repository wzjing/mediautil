//
// Created by android1 on 2019/5/29.
//

#include "transcode.h"
#include "log.h"

#define TAG "transcode"

#define checkret(ret, msg) if(ret < 0){LOGD(TAG, "%s: %s\n", msg, av_err2str(ret));return -1;}
#define nonnull(ptr, msg) if(!ptr){LOGD(TAG, "null pointer: %s\n", msg);return -1;}

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
    checkret(ret, "unable to open input file")
    ret = avformat_find_stream_info(inFmtCtx, nullptr);
    checkret(ret, "unable to get input file info")

    // open output file
    ret = avformat_alloc_output_context2(&outFmtCtx, nullptr, nullptr, output_filename);
    checkret(ret, "unable to open output file")

    for (int i = 0; i < inFmtCtx->nb_streams; ++i) {
        AVStream *inStream = inFmtCtx->streams[i];
        if (inStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AVCodec *decoder = avcodec_find_decoder(inStream->codecpar->codec_id);
            nonnull(decoder, "unable to find audio decoder")
            aDecCtx = avcodec_alloc_context3(decoder);
            nonnull(aDecCtx, "unable to alloc audio decode context")
            ret = avcodec_parameters_to_context(aDecCtx, inStream->codecpar);
            checkret(ret, "unable to set audio decode context")
            ret = avcodec_open2(aDecCtx, decoder, nullptr);
            checkret(ret, "unable to open audio decode context")

            AVCodec *encoder = avcodec_find_encoder(new_config->audio_codec_id);
            nonnull(decoder, "unable to find audio encoder")
            AVStream *outStream = avformat_new_stream(outFmtCtx, encoder);
            nonnull(outStream, "unable to create output audio stream")
            outStream->id = outFmtCtx->nb_streams - 1;
            aEncCtx = avcodec_alloc_context3(decoder);
            nonnull(outStream, "unable to create output audio stream")
            aEncCtx->codec_id = encoder->id;
            aEncCtx->sample_fmt = new_config->sample_fmt ? new_config->sample_fmt : aDecCtx->sample_fmt;
            aEncCtx->sample_rate = new_config->sample_rate ? new_config->sample_rate : aDecCtx->sample_rate;
            aEncCtx->channel_layout = new_config->channel_layout;
            aEncCtx->bit_rate = new_config->audio_bitrate ? new_config->audio_bitrate : aDecCtx->bit_rate;
            aEncCtx->time_base = (AVRational) {1, aEncCtx->sample_rate};
            outStream->time_base = aEncCtx->time_base;
            if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
                aEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            ret = avcodec_open2(aEncCtx, encoder, nullptr);
            checkret(ret, "unable to create output audio encode context")
            ret = avcodec_parameters_from_context(outStream->codecpar, aEncCtx);
            checkret(ret, "unable to copy parameter from audio context to stream")
            av_dict_copy(&outStream->metadata, inStream->metadata, 0);

            logStream(inStream, "IN", 0);
            logStream(outStream, "OUT", 0);

        } else if (inStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVCodec *decoder = avcodec_find_decoder(inStream->codecpar->codec_id);
            nonnull(decoder, "unable to find video decoder")
            vDecCtx = avcodec_alloc_context3(decoder);
            nonnull(aDecCtx, "unable to alloc video decode context")
            ret = avcodec_parameters_to_context(vDecCtx, inStream->codecpar);
            checkret(ret, "unable to set video decode context")
            ret = avcodec_open2(vDecCtx, decoder, nullptr);
            checkret(ret, "unable to open video decode context")

            AVCodec *encoder = avcodec_find_encoder(new_config->video_codec_id);
            AVStream *outStream = avformat_new_stream(outFmtCtx, encoder);
            nonnull(outStream, "unable to create output video stream")
            outStream->id = outFmtCtx->nb_streams - 1;
            vEncCtx = avcodec_alloc_context3(decoder);
            nonnull(outStream, "unable to create output audio stream")
            vEncCtx->codec_id = encoder->id;
            vEncCtx->pix_fmt = new_config->pix_fmt ? new_config->pix_fmt : vEncCtx->pix_fmt;
            vEncCtx->width = new_config->width ? new_config->width : vEncCtx->width;
            vEncCtx->height = new_config->height ? new_config->height : vEncCtx->height;
            vEncCtx->bit_rate = new_config->audio_bitrate ? new_config->audio_bitrate : vEncCtx->bit_rate;
            vEncCtx->framerate = vDecCtx->framerate;
            vEncCtx->gop_size = vEncCtx->framerate.num;
            vEncCtx->qmax = 32;
            vEncCtx->qmin = 2;
            aEncCtx->time_base = (AVRational) {aEncCtx->framerate.den, aEncCtx->framerate.num};
            outStream->time_base = aEncCtx->time_base;
            AVDictionary *opt = nullptr;
            if (vEncCtx->codec_id == AV_CODEC_ID_H264) {
                av_dict_set(&opt, "preset", "fast", 0);
                av_dict_set(&opt, "tune", "zerolatency", 0);
            }
            if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
                vEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            ret = avcodec_open2(vEncCtx, encoder, &opt);
            av_dict_free(&opt);
            checkret(ret, "unable to create output audio encode context")
            ret = avcodec_parameters_from_context(outStream->codecpar, vEncCtx);
            checkret(ret, "unable to copy parameter from audio context to stream")
            av_dict_copy(&outStream->metadata, inStream->metadata, 0);

            logStream(inStream, "IN", 1);
            logStream(outStream, "OUT", 1);
        }
    }

    logContext(aDecCtx, "IN", 0);
    logContext(aEncCtx, "OUT", 0);

    logContext(vDecCtx, "IN", 1);
    logContext(vEncCtx, "OUT", 1);

    AVFrame *inFrame = av_frame_alloc();
    AVFrame *outFrame = av_frame_alloc();

    int encode_video = 1;
    int encode_audio = 1;
    int64_t video_pts = 0;
    int64_t audio_pts = 0;

    int got_packet = 0;

    while (true) {
        AVPacket inPacket{nullptr};
        av_init_packet(&inPacket);
        ret = av_read_frame(inFmtCtx, &inPacket);
        if (ret == 0) {
            if (inPacket.stream_index = aOutStream->index) {
                ret = avcodec_send_packet(aDecCtx, &inPacket);
                if (ret == 0) {}

            } else if (inPacket.stream_index = vOutStream->index) {

            }
        }
    }

    return 0;
}
