
#include "clip.h"

#include "log.h"
#include "video_filter.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#define TAG "clip"

#define check(ret, msg) if(ret < 0) {\
                            LOGE(TAG, "%s: %s", msg, av_err2str(ret));\
                            return -1;\
                        }

int clip(const char *output_filename, const char *input_filename, float from, float to) {

    if (from < 0 || from >= to) {
        LOGE(TAG, "invalid time: %f -> %f\n", from, to);
        return -1;
    }

    AVFormatContext *inFmtCtx = nullptr;
    AVFormatContext *outFmtCtx = nullptr;
    AVCodecContext *videoCodecCtx = nullptr;
    AVCodecContext *gifCodecCtx = nullptr;

    int ret = 0;

    ret = avformat_open_input(&inFmtCtx, input_filename, nullptr, nullptr);
    check(ret, "open input error")
    ret = avformat_find_stream_info(inFmtCtx, nullptr);
    check(ret, "find input info error")

    ret = avformat_alloc_output_context2(&outFmtCtx, nullptr, nullptr, output_filename);
    check(ret, "open output error")

    int isGif = strcmp(outFmtCtx->oformat->name, "gif") == 0;

    int stream_map[inFmtCtx->nb_streams];

    int video_idx = 0;

    for (int i = 0; i < inFmtCtx->nb_streams; ++i) {
        if (inFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (isGif) continue;
            AVStream *inAudioStream = inFmtCtx->streams[i];
            AVStream *outAudioStream = avformat_new_stream(outFmtCtx, nullptr);
            check(ret, "unable to create output audio stream")
            ret = avcodec_parameters_copy(outAudioStream->codecpar, inAudioStream->codecpar);
            check(ret, "copy audio parameter error")
            stream_map[i] = outFmtCtx->nb_streams - 1;
            av_dict_copy(&outAudioStream->metadata, inAudioStream->metadata, 0);
        } else if (inFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_idx = i;
            AVStream *inVideoStream = inFmtCtx->streams[i];
            AVStream *outVideoStream = avformat_new_stream(outFmtCtx, nullptr);
            check(ret, "unable to create output video stream")
            stream_map[i] = outFmtCtx->nb_streams - 1;
            av_dict_copy(&outVideoStream->metadata, inVideoStream->metadata, 0);

            if (isGif) {
                const AVCodec *inCodec = avcodec_find_decoder(inVideoStream->codecpar->codec_id);
                videoCodecCtx = avcodec_alloc_context3(inCodec);
                ret = avcodec_parameters_to_context(videoCodecCtx, inVideoStream->codecpar);
                check(ret, "unable to copy parameter to decode context")
                ret = avcodec_open2(videoCodecCtx, inCodec, nullptr);
                check(ret, "unable to open decode context")

                const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_GIF);
                gifCodecCtx = avcodec_alloc_context3(codec);
                gifCodecCtx->codec_id = AV_CODEC_ID_GIF;
                gifCodecCtx->time_base = {1, 30};
                gifCodecCtx->pix_fmt = AV_PIX_FMT_RGB8;
                gifCodecCtx->width = videoCodecCtx->width;
                gifCodecCtx->height = videoCodecCtx->height;
                check(ret, "error while copy parameter")
                ret = avcodec_open2(gifCodecCtx, codec, nullptr);
                check(ret, "unable to open gif encode context")
                ret = avcodec_parameters_from_context(outVideoStream->codecpar, gifCodecCtx);
                check(ret, "unable to copy parameter to stream")
            } else {
                ret = avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar);
                check(ret, "copy video parameter error")
            }
        }
    }
    av_dict_copy(&outFmtCtx->metadata, inFmtCtx->metadata, 0);

    if (!(outFmtCtx->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outFmtCtx->pb, output_filename, AVIO_FLAG_WRITE);
        check(ret, "could not open output file")
    }

    ret = avformat_write_header(outFmtCtx, nullptr);
    check(ret, "could not write header")

    int64_t first_pts = av_rescale_q_rnd((int64_t) from, (AVRational) {1, 1},
                                         inFmtCtx->streams[video_idx]->time_base,
                                         AV_ROUND_DOWN);
    LOGD(TAG, "seek(%d): %ld\n", video_idx, first_pts);
    av_seek_frame(inFmtCtx, video_idx, first_pts, AVSEEK_FLAG_BACKWARD);

    VideoFilter *filter = nullptr;
    if (isGif) {
        filter = new VideoFilter();
        char filter_descr[128];
        snprintf(filter_descr, sizeof(filter_descr), "[in]format=pix_fmts=%s[out]",
                 av_get_pix_fmt_name(gifCodecCtx->pix_fmt));
        VideoConfig in(videoCodecCtx->pix_fmt, videoCodecCtx->width, videoCodecCtx->height);
        VideoConfig out(gifCodecCtx->pix_fmt, gifCodecCtx->width, gifCodecCtx->height);
        filter->create(filter_descr, &in, &out);
    }

    int gif_pts = 0;
    while (true) {
        AVPacket packet{0};
        av_init_packet(&packet);
        ret = av_read_frame(inFmtCtx, &packet);
        if (ret < 0) {
            LOGW(TAG, "read: %s\n", av_err2str(ret));
            break;
        }
        if (av_compare_ts(packet.pts, inFmtCtx->streams[packet.stream_index]->time_base,
                          (int64_t) (to * 10), (AVRational) {1, 10}) >= 0) {
            break;
        }

        if (packet.stream_index == video_idx)
            logPacket(&packet, &inFmtCtx->streams[packet.stream_index]->time_base, "pkt");

        if (isGif) {
            if (packet.stream_index == video_idx) {

                AVFrame *frame = av_frame_alloc();
                ret = avcodec_send_packet(videoCodecCtx, &packet);
                if (ret < 0) {
                    LOGW(TAG, "decode send: %s\n", av_err2str(ret));
                    continue;
                }
                ret = avcodec_receive_frame(videoCodecCtx, frame);
                if (ret < 0) {
                    LOGW(TAG, "decode receive: %s\n", av_err2str(ret));
                    continue;
                }

                filter->filter(frame, frame);
                frame->pts = gif_pts++;

                ret = avcodec_send_frame(gifCodecCtx, frame);
                if (ret < 0) {
                    LOGW(TAG, "encode send: %s\n", av_err2str(ret));
                    continue;
                }
                av_frame_free(&frame);

                AVPacket gifPkt{0};
                av_init_packet(&gifPkt);
                ret = avcodec_receive_packet(gifCodecCtx, &gifPkt);
                if (ret < 0) {
                    LOGW(TAG, "encode receive: %s\n", av_err2str(ret));
                    continue;
                }

                logPacket(&gifPkt, &outFmtCtx->streams[video_idx]->time_base, "gif");

                gifPkt.stream_index = stream_map[packet.stream_index];
                av_packet_rescale_ts(&gifPkt, gifCodecCtx->time_base,
                                     outFmtCtx->streams[stream_map[video_idx]]->time_base);
                logPacket(&gifPkt, &outFmtCtx->streams[video_idx]->time_base, "gif-s");
                ret = av_interleaved_write_frame(outFmtCtx, &gifPkt);
                check(ret, "unable to write gif frame")
            }
        } else {
            packet.stream_index = stream_map[packet.stream_index];
            av_packet_rescale_ts(&packet, inFmtCtx->streams[packet.stream_index]->time_base,
                                 outFmtCtx->streams[stream_map[packet.stream_index]]->time_base);
            av_interleaved_write_frame(outFmtCtx, &packet);
        }
    }

    if (!isGif) {
        av_write_trailer(outFmtCtx);
    }

    if (filter != nullptr) {
        filter->destroy();
        delete filter;
    }

    avformat_close_input(&inFmtCtx);
    avformat_free_context(inFmtCtx);
    avformat_free_context(outFmtCtx);
    return 0;
}
