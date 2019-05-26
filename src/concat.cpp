#include "log.h"
#include "video_filter.h"

#include "concat.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/avutil.h>
}

struct Video {
    AVFormatContext *formatContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    AVCodecContext *audioCodecContext = nullptr;
    AVCodecContext *videoCodecContext = nullptr;
    int isTsVideo = 0;

};

#define TAG "concat"

static int titleDuration = 2;
static int fontSize = 40;

int encode_title(const char *title, AVFormatContext *formatContext,
                 AVFrame *srcAudioFrame, AVFrame *srcVideoFrame,
                 AVCodecContext *audioCodecContext, AVCodecContext *videoCodecContext,
                 AVStream *audioStream, AVStream *videoStream,
                 uint64_t &audio_start_pts, uint64_t &audio_start_dts,
                 uint64_t &video_start_pts, uint64_t &video_start_dts) {

    int encode_video = 1;
    int encode_audio = 1;
    int ret = 0;

    AVFrame *videoFrame = av_frame_alloc();
    AVFrame *audioFrame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    videoFrame->width = videoCodecContext->width;
    videoFrame->height = videoCodecContext->height;
    videoFrame->format = videoCodecContext->pix_fmt;
    ret = av_frame_get_buffer(videoFrame, 0);
    if (ret < 0) {
        LOGW(TAG, "warning: unable to get video buffers");
    }

    int64_t video_frame_pts = 0;
    int64_t audio_frame_pts = 0;

    uint64_t next_video_pts = 0;
    uint64_t next_video_dts = 0;
    uint64_t next_audio_pts = 0;
    uint64_t next_audio_dts = 0;

    int sample_size = 0;

    int first_video_set = 0;
    int first_audio_set = 0;

    VideoFilter filter;
    char filter_description[512];
    snprintf(filter_description, 512,
             "gblur=sigma=20:steps=6[blur];"
             "color=black:%dx%d[canvas];"
             "[canvas]setsar=1,drawtext=fontsize=%d:fontcolor=white:text='%s':x=w/2-text_w/2:y=h/2-text_h/2,split[text][alpha];"
             "[text][alpha]alphamerge,rotate=3*PI/2[ov];"
             "[blur][ov]overlay=w/2:0",
             srcVideoFrame->height,
             srcVideoFrame->height,
             fontSize, title);
    LOGD(TAG, "filter: %s\n", filter_description);
    VideoConfig inConfig((AVPixelFormat) srcVideoFrame->format, srcVideoFrame->width, srcVideoFrame->height);
    VideoConfig outConfig((AVPixelFormat) videoFrame->format, videoFrame->width, videoFrame->height);
    ret = filter.create(filter_description, &inConfig, &outConfig);

    if (ret <0) {
        LOGE(TAG, "unable to create filter");
        goto error;
    }

    filter.dumpGraph();

    ret = filter.filter(srcVideoFrame, srcVideoFrame);
    if (ret < 0) {
        LOGE(TAG, "unable to filter frame\n");
        goto error;
    }
    filter.destroy();

    sample_size = av_get_bytes_per_sample((AVSampleFormat) srcAudioFrame->format);
    for (int i = 0; i < srcAudioFrame->channels; i++) {
        memset(srcAudioFrame->data[i], '0', srcAudioFrame->nb_samples * sample_size);
    }

    while (encode_video || encode_audio) {
        if (!encode_audio || (encode_video && av_compare_ts(video_frame_pts, videoCodecContext->time_base,
                                                            audio_frame_pts, audioCodecContext->time_base) <= 0)) {
            if (av_compare_ts(video_frame_pts, videoCodecContext->time_base,
                              titleDuration, (AVRational) {1, 1}) > 0) {
                encode_video = 0;
            } else {
                ret = av_frame_copy(videoFrame, srcVideoFrame);
                if (ret < 0) LOGW(TAG, "\tget copy video frame failed\n");
                ret = av_frame_copy_props(videoFrame, srcAudioFrame);
                if (ret < 0) LOGW(TAG, "\tget copy video frame props failed\n");
                if (!first_video_set) {
                    videoFrame->pict_type = AV_PICTURE_TYPE_I;
                    first_video_set = 1;
                } else {
                    videoFrame->pict_type = AV_PICTURE_TYPE_NONE;
                }
                videoFrame->pts = video_frame_pts;
                video_frame_pts += 1;
                avcodec_send_frame(videoCodecContext, videoFrame);
            }
            while (true) {
                ret = avcodec_receive_packet(videoCodecContext, packet);
                if (ret == 0) {
                    av_packet_rescale_ts(packet, videoCodecContext->time_base, videoStream->time_base);
                    packet->stream_index = videoStream->index;
                    packet->pts += video_start_pts;
                    packet->dts += video_start_dts;
                    next_video_pts = packet->pts + srcVideoFrame->pkt_duration;
                    next_video_dts = packet->dts + srcVideoFrame->pkt_duration;
#ifdef DEBUG
                    logPacket(packet, &videoStream->time_base, "V");
#endif
                    ret = av_interleaved_write_frame(formatContext, packet);
                    if (ret < 0) {
                        LOGE(TAG, "write video frame error: %s\n", av_err2str(ret));
                        goto error;
                    }
                } else if (ret == AVERROR(EAGAIN)) {
                    break;
                } else if (ret == AVERROR_EOF) {
                    LOGW(TAG, "Stream Video finished\n");
                    encode_video = 0;
                    break;
                } else {
                    LOGE(TAG, "encode video frame error: %s\n", av_err2str(ret));
                    goto error;
                }
            }
        } else {
            if (av_compare_ts(audio_frame_pts, audioCodecContext->time_base,
                              titleDuration, (AVRational) {1, 1}) >= 0) {
                encode_audio = 0;
            } else {
                audioFrame->format = audioCodecContext->sample_fmt;
                audioFrame->nb_samples = srcAudioFrame->nb_samples;
                audioFrame->sample_rate = audioCodecContext->sample_rate;
                audioFrame->channel_layout = audioCodecContext->channel_layout;
                ret = av_frame_get_buffer(audioFrame, 0);
                if (ret < 0) LOGW(TAG, "\tget audio buffer failed\n");
                ret = av_frame_copy(audioFrame, srcAudioFrame);
                if (ret < 0) LOGW(TAG, "\tcopy audio failed\n");
                av_frame_copy_props(audioFrame, srcAudioFrame);
                if (ret < 0) LOGW(TAG, "\tcopy audio prop failed\n");
                audioFrame->pict_type = AV_PICTURE_TYPE_NONE;
                audioFrame->pts = audio_frame_pts;
                audio_frame_pts += srcAudioFrame->nb_samples;
                avcodec_send_frame(audioCodecContext, audioFrame);
            }
            while (true) {
                ret = avcodec_receive_packet(audioCodecContext, packet);
                if (ret == 0) {
                    if (packet->pts == 0) {
                        first_audio_set = 1;
                    }
                    if (!first_audio_set) continue;
                    av_packet_rescale_ts(packet, audioCodecContext->time_base, audioStream->time_base);
                    packet->stream_index = audioStream->index;
                    packet->pts += audio_start_pts;
                    packet->dts += audio_start_dts;
                    next_audio_pts = packet->pts + srcAudioFrame->pkt_duration;
                    next_audio_dts = packet->dts + srcAudioFrame->pkt_duration;
#ifdef DEBUG
                    logPacket(packet, &audioStream->time_base, "A");
#endif
                    ret = av_interleaved_write_frame(formatContext, packet);
                    if (ret < 0) {
                        LOGE(TAG, "write audio frame error: %s\n", av_err2str(ret));
                        goto error;
                    }
                    break;
                } else if (ret == AVERROR(EAGAIN)) {
                    break;
                } else if (ret == AVERROR_EOF) {
                    LOGW(TAG, "Stream Audio finished\n");
                    encode_audio = 0;
                    break;
                } else {
                    LOGE(TAG, "encode audio frame error: %s\n", av_err2str(ret));
                    goto error;
                }
            }
        }
    }

    video_start_pts = next_video_pts;
    video_start_dts = next_video_dts;
    audio_start_pts = next_audio_pts;
    audio_start_dts = next_audio_dts;

    av_frame_free(&audioFrame);
    av_frame_free(&videoFrame);
    av_packet_free(&packet);
    return 0;
    error:
    av_frame_free(&audioFrame);
    av_frame_free(&videoFrame);
    av_packet_free(&packet);
    return -1;
}

int concat_add_title(const char *output_filename, const char **input_filenames, const char** titles, int nb_inputs,
                     int font_size,
                     int title_duration) {

    fontSize = font_size;
    titleDuration = title_duration;

    int ret = 0;
    // input fragments
    auto **videos = (Video **) malloc(nb_inputs * sizeof(Video *));

    for (int i = 0; i < nb_inputs; ++i) {
        videos[i] = (Video *) malloc(sizeof(Video));
        videos[i]->formatContext = nullptr;
    }

    // output video
    AVFormatContext *outFmtContext = nullptr;
    AVStream *outVideoStream = nullptr;
    AVStream *outAudioStream = nullptr;
    AVCodecContext *outVideoContext = nullptr;
    AVCodecContext *outAudioContext = nullptr;
    AVCodec *outVideoCodec = nullptr;
    AVCodec *outAudioCodec = nullptr;

    for (int i = 0; i < nb_inputs; ++i) {
        ret = avformat_open_input(&videos[i]->formatContext, input_filenames[i], nullptr, nullptr);
        if (ret < 0) {
            LOGE(TAG, "open file %s error: %s\n", input_filenames[i], av_err2str(ret));
            return -1;
        }
        ret = avformat_find_stream_info(videos[i]->formatContext, nullptr);
        if (ret < 0) {
            LOGE(TAG, "unable to find stream info for %s: %s\n", input_filenames[i], av_err2str(ret));
            return -1;
        }
        for (int j = 0; j < videos[i]->formatContext->nb_streams; ++j) {
            if (videos[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videos[i]->videoStream = videos[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(videos[i]->videoStream->codecpar->codec_id);
                videos[i]->videoCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(videos[i]->videoCodecContext, videos[i]->videoStream->codecpar);
                avcodec_open2(videos[i]->videoCodecContext, codec, nullptr);
            } else if (videos[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                videos[i]->audioStream = videos[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(videos[i]->audioStream->codecpar->codec_id);
                videos[i]->audioCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(videos[i]->audioCodecContext, videos[i]->audioStream->codecpar);
                avcodec_open2(videos[i]->audioCodecContext, codec, nullptr);
            }
            if (videos[i]->videoStream && videos[i]->audioStream) break;
        }
        if (videos[i]->formatContext->iformat->name) {

        }
        videos[i]->isTsVideo = strcmp(videos[i]->formatContext->iformat->name, "mpegts") == 0;

        LOGD(TAG, "\n%s:\t%s/%s -> %s\n\n",
             videos[i]->isTsVideo ? "TS" : "--",
             videos[i]->videoStream ? "Video" : "--",
             videos[i]->audioStream ? "Audio" : "--",
             input_filenames[i]);
    }

    // create output AVFormatContext
    ret = avformat_alloc_output_context2(&outFmtContext, nullptr, nullptr, output_filename);
    if (ret < 0) {
        LOGE(TAG, "");
        return -1;
    }

    // Copy codec from input video AVStream
    outVideoCodec = avcodec_find_encoder(videos[0]->videoStream->codecpar->codec_id);
    outAudioCodec = avcodec_find_encoder(videos[0]->audioStream->codecpar->codec_id);

    // create output Video AVStream
    outVideoStream = avformat_new_stream(outFmtContext, outVideoCodec);
    if (!outVideoStream) {
        LOGE(TAG, "unable to create output video stream: %s\n", av_err2str(ret));
        return -1;
    }
    outVideoStream->id = outFmtContext->nb_streams - 1;

    outAudioStream = avformat_new_stream(outFmtContext, outAudioCodec);
    if (!outAudioStream) {
        LOGE(TAG, "unable to create output audio stream: %s\n", av_err2str(ret));
        return -1;
    }
    outAudioStream->id = outFmtContext->nb_streams - 1;

    Video *baseVideo = videos[0]; // the output video format is based on the first input video

    // Copy Video Stream Configure from base Video
    outVideoContext = avcodec_alloc_context3(outVideoCodec);
    outVideoContext->codec_id = baseVideo->videoCodecContext->codec_id;
    outVideoContext->width = baseVideo->videoCodecContext->width;
    outVideoContext->height = baseVideo->videoCodecContext->height;
    outVideoContext->pix_fmt = baseVideo->videoCodecContext->pix_fmt;
    outVideoContext->bit_rate = baseVideo->videoCodecContext->bit_rate;
    outVideoContext->has_b_frames = baseVideo->videoCodecContext->has_b_frames;
    outVideoContext->gop_size = 30;
    outVideoContext->qmin = baseVideo->videoCodecContext->qmin;
    outVideoContext->qmax = baseVideo->videoCodecContext->qmax;
    outVideoContext->time_base = (AVRational) {baseVideo->videoStream->r_frame_rate.den,
                                               baseVideo->videoStream->r_frame_rate.num};
    outVideoContext->profile = baseVideo->videoCodecContext->profile;
    outVideoStream->time_base = outVideoContext->time_base;
    AVDictionary *opt = nullptr;
    if (outVideoContext->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&opt, "preset", "fast", 0);
        av_dict_set(&opt, "tune", "zerolatency", 0);
    }
    if (outFmtContext->oformat->flags & AVFMT_GLOBALHEADER)
        outVideoContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    ret = avcodec_open2(outVideoContext, outVideoCodec, &opt);
    if (ret < 0) {
        LOGE(TAG, "unable to create output video codec context: %s\n", av_err2str(ret));
        return -1;
    }
    ret = avcodec_parameters_from_context(outVideoStream->codecpar, outVideoContext);
    if (ret < 0) {
        LOGE(TAG, "unable to copy parameter to output video stream: %s\n", av_err2str(ret));
        return -1;
    }
    ret = av_dict_copy(&outVideoStream->metadata, baseVideo->videoStream->metadata, 0);
    if (ret < 0) LOGW(TAG, "failed copy metadata: %s\n", av_err2str(ret));


    // Copy Audio Stream Configure from base Video
    outAudioContext = avcodec_alloc_context3(outAudioCodec);
    outAudioContext->codec_type = baseVideo->audioCodecContext->codec_type;
    outAudioContext->codec_id = baseVideo->audioCodecContext->codec_id;
    outAudioContext->sample_fmt = baseVideo->audioCodecContext->sample_fmt;
    outAudioContext->sample_rate = baseVideo->audioCodecContext->sample_rate;
    outAudioContext->bit_rate = baseVideo->audioCodecContext->bit_rate;
    outAudioContext->channel_layout = baseVideo->audioCodecContext->channel_layout;
    outAudioContext->channels = baseVideo->audioCodecContext->channels;
    outAudioContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
    outAudioContext->time_base = (AVRational) {1, outAudioContext->sample_rate};
    outAudioStream->time_base = outAudioContext->time_base;
    av_dict_free(&opt);
//    opt = nullptr;
//    if (outVideoContext->codec_id == AV_CODEC_ID_AAC) {
//        av_dict_set(&opt, "profile", "23", 0);
//    }
    if (outFmtContext->oformat->flags & AVFMT_GLOBALHEADER)
        outAudioContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    ret = avcodec_open2(outAudioContext, outAudioCodec, nullptr);
    if (ret < 0) {
        LOGE(TAG, "unable to create output audio codec context: %s\n", av_err2str(ret));
        return -1;
    }
    ret = avcodec_parameters_from_context(outAudioStream->codecpar, outAudioContext);
    if (ret < 0) {
        LOGE(TAG, "unable to copy parameter to output audio stream: %s\n", av_err2str(ret));
        return -1;
    }

    if (!(outFmtContext->oformat->flags & AVFMT_NOFILE)) {
        LOGD(TAG, "Opening file: %s\n", output_filename);
        ret = avio_open(&outFmtContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE(TAG, "could not open %s (%s)\n", output_filename, av_err2str(ret));
            return -1;
        }
    }

    ret = avformat_write_header(outFmtContext, nullptr);
    if (ret < 0) {
        LOGE(TAG, "unable to write output file header: %s\n", av_err2str(ret));
        return -1;
    }

    av_dump_format(outFmtContext, 0, output_filename, 1);

    AVPacket *packet = av_packet_alloc();
    AVFrame *videoFrame = av_frame_alloc();
    AVFrame *audioFrame = av_frame_alloc();

    uint64_t last_video_pts = 0;
    uint64_t last_video_dts = 0;
    uint64_t last_audio_pts = 0;
    uint64_t last_audio_dts = 0;


    for (int i = 0; i < nb_inputs; ++i) {
        AVFormatContext *inFormatContext = videos[i]->formatContext;

        AVStream *inVideoStream = videos[i]->videoStream;
        AVStream *inAudioStream = videos[i]->audioStream;
        AVCodecContext *audioContext = videos[i]->audioCodecContext;
        AVCodecContext *videoContext = videos[i]->videoCodecContext;
        AVBSFContext *bsfContext = nullptr;

        if (!videos[i]->isTsVideo) {
            // bitstream filter: convert mp4 packet to annexb
            const AVBitStreamFilter *bsfFilter = av_bsf_get_by_name("h264_mp4toannexb");

            if (!bsfFilter) {
                LOGE(TAG, "unable to find bsf tiler\n");
                return -1;
            }
            ret = av_bsf_alloc(bsfFilter, &bsfContext);
            if (ret < 0) {
                LOGE(TAG, "unable to create bsf context");
                return -1;
            }
            ret = avcodec_parameters_from_context(bsfContext->par_in, videoContext);
            if (ret < 0) {
                LOGE(TAG, "unable to copy in parameters to bsf context");
                return -1;
            }
            bsfContext->time_base_in = inVideoStream->time_base;
            ret = av_bsf_init(bsfContext);
            if (ret < 0) {
                LOGE(TAG, "unable to init bsf context");
                return -1;
            }

            if (ret < 0) {
                LOGE(TAG, "unable to copy out parameters to bsf context");
                return -1;
            }
            LOGD(TAG, "bsf timebase: {%d, %d} -> {%d, %d}\n", inVideoStream->time_base.num,
                 inVideoStream->time_base.den,
                 bsfContext->time_base_out.num, bsfContext->time_base_out.den);
        }

        uint64_t first_video_pts = 0;
        uint64_t first_video_dts = 0;
        uint64_t first_audio_pts = 0;
        uint64_t first_audio_dts = 0;
        int video_ts_set = 0;
        int audio_ts_set = 0;

        uint64_t next_video_pts = 0;
        uint64_t next_video_dts = 0;
        uint64_t next_audio_pts = 0;
        uint64_t next_audio_dts = 0;


        // use first frame to make a title
        int got_video = 0;
        int got_audio = 0;
        do {
            ret = av_read_frame(inFormatContext, packet);
            if (ret < 0) break;
            if (!got_video && packet->stream_index == inVideoStream->index) {
                ret = avcodec_send_packet(videoContext, packet);
                if (ret < 0) continue;
                ret = avcodec_receive_frame(videoContext, videoFrame);
                if (ret < 0) continue;
                else got_video = 1;
            } else if (!got_audio && packet->stream_index == inAudioStream->index) {
                ret = avcodec_send_packet(audioContext, packet);
                if (ret < 0) continue;
                ret = avcodec_receive_frame(audioContext, audioFrame);
                if (ret < 0) continue;
                else got_audio = 1;
            }
        } while (!got_video || !got_audio);

        if (!got_video) {
            LOGW(TAG, "unable to get input video frame\n");
            videoFrame->width = outVideoContext->width;
            videoFrame->height = outVideoContext->height;
            videoFrame->format = outVideoContext->pix_fmt;
            av_frame_get_buffer(videoFrame, 0);
        }

        if (!got_audio) {
            LOGW(TAG, "unable to get input audio frame\n");
            audioFrame->nb_samples = 1024;
            audioFrame->sample_rate = outAudioContext->sample_rate;
        }

        ret = encode_title(titles[i], outFmtContext, audioFrame, videoFrame, outAudioContext, outVideoContext,
                     outAudioStream, outVideoStream, last_audio_pts, last_audio_dts, last_video_pts, last_video_dts);

        if (ret < 0) {
            LOGE(TAG, "encode title failed for file: %s\n", input_filenames[i]);
            return -1;
        }

        av_seek_frame(inFormatContext, inAudioStream->index, 0, 0);
        av_seek_frame(inFormatContext, inVideoStream->index, 0, 0);

        LOGD(TAG, "\nlast timestamp: A(%lld/%lld) V(%lld/%lld)\n\n", last_audio_pts, last_audio_dts, last_video_pts,
             last_video_dts);

        // copy the video file
        do {
            ret = av_read_frame(inFormatContext, packet);
            if (ret == AVERROR_EOF) {
                LOGW(TAG, "\tread fragment end of file\n");
                break;
            } else if (ret < 0) {
                LOGE(TAG, "read fragment error: %s\n", av_err2str(ret));
                break;
            }

            if (packet->dts < 0) continue;
            if (packet->stream_index == inVideoStream->index) {
                if (packet->flags & AV_PKT_FLAG_DISCARD) {
                    LOGW(TAG, "\nPacket is discard\n");
                    continue;
                }

                if (!videos[i]->isTsVideo) {
                    AVPacket *annexPacket = av_packet_alloc();
                    ret = av_bsf_send_packet(bsfContext, packet);
                    if (ret < 0) LOGW(TAG, "unable to convert packet to annexb: %s\n", av_err2str(ret));
                    ret = av_bsf_receive_packet(bsfContext, annexPacket);
                    if (ret != 0) LOGW(TAG, "unable to receive converted annexb packet: %s\n", av_err2str(ret));
//                    LOGI(TAG, "\t mp4 to annexb\n");
                    if (!video_ts_set) {
                        first_video_pts = annexPacket->pts;
                        first_video_dts = annexPacket->dts;
                        video_ts_set = 1;
                    }
                    annexPacket->stream_index = outVideoStream->index;

                    annexPacket->pts -= first_video_pts;
                    annexPacket->dts -= first_video_dts;
//                    av_packet_rescale_ts(annexPacket, bsfContext->time_base_out, outVideoStream->time_base);
                    annexPacket->pts = av_rescale_q_rnd(annexPacket->pts, bsfContext->time_base_out,
                                                        outVideoStream->time_base,
                                                        (AVRounding) (AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
                    annexPacket->dts = av_rescale_q_rnd(annexPacket->dts, bsfContext->time_base_out,
                                                        outVideoStream->time_base,
                                                        (AVRounding) (AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
                    annexPacket->duration = av_rescale_q(annexPacket->duration, bsfContext->time_base_out,
                                                         outVideoStream->time_base);
                    annexPacket->pos = -1;
                    annexPacket->pts += last_video_pts;
                    annexPacket->dts += last_video_dts;
                    next_video_pts = annexPacket->pts + annexPacket->duration;
                    next_video_dts = annexPacket->dts + annexPacket->duration;
                    annexPacket->flags = packet->flags;

#ifdef DEBUG
                    logPacket(annexPacket, &outVideoStream->time_base, "Annex");
#endif
                    av_interleaved_write_frame(outFmtContext, annexPacket);
                    av_packet_free(&annexPacket);
                } else {
                    if (!video_ts_set) {
                        first_video_pts = packet->pts;
                        first_video_dts = packet->dts;
                        video_ts_set = 1;
                    }

                    packet->stream_index = outVideoStream->index;

                    packet->pts -= first_video_pts;
                    packet->dts -= first_video_dts;
//                    av_packet_rescale_ts(packet, inVideoStream->time_base, outVideoStream->time_base);
                    packet->pts = av_rescale_q_rnd(packet->pts, inVideoStream->time_base, outVideoStream->time_base,
                                                   (AVRounding) (AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
                    packet->dts = av_rescale_q_rnd(packet->dts, inVideoStream->time_base, outVideoStream->time_base,
                                                   (AVRounding) (AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
                    packet->duration = av_rescale_q(packet->duration, inVideoStream->time_base,
                                                    outVideoStream->time_base);
                    packet->pos = -1;
                    packet->pts += last_video_pts;
                    packet->dts += last_video_dts;
                    next_video_pts = packet->pts + packet->duration;
                    next_video_dts = packet->dts + packet->duration;

                    logPacket(packet, &outVideoStream->time_base, "V");

                    av_interleaved_write_frame(outFmtContext, packet);
                }

            } else if (packet->stream_index == inAudioStream->index) {

                packet->stream_index = outAudioStream->index;

                if (!audio_ts_set) {
                    first_audio_pts = packet->pts;
                    first_audio_dts = packet->dts;
                    audio_ts_set = 1;
                }
                packet->pts -= first_audio_pts;
                packet->dts -= first_audio_dts;
                av_packet_rescale_ts(packet, inAudioStream->time_base, outAudioStream->time_base);
                packet->pts += last_audio_pts;
                packet->dts += last_audio_dts;
                next_audio_pts = packet->pts + packet->duration;
                next_audio_dts = packet->dts + packet->duration;
#ifdef DEBUG
                logPacket(packet, &outAudioStream->time_base, "A");
#endif
                av_interleaved_write_frame(outFmtContext, packet);
            }
        } while (true);
        last_video_pts = next_video_pts;
        last_video_dts = next_video_dts;
        last_audio_pts = next_audio_pts;
        last_audio_dts = next_audio_dts;

        if (!videos[i]->isTsVideo) av_bsf_free(&bsfContext);

        LOGD(TAG, "--------------------------------------------------------------------------------\n");
    }


    av_write_trailer(outFmtContext);


    if (!(outFmtContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outFmtContext->pb);
    }


    av_packet_free(&packet);
    avformat_free_context(outFmtContext);

    for (int i = 0; i < nb_inputs; ++i) {
        avformat_free_context(videos[i]->formatContext);
        if (videos[i]->videoCodecContext)
            avcodec_free_context(&videos[i]->videoCodecContext);
        if (videos[i]->audioCodecContext)
            avcodec_free_context(&videos[i]->audioCodecContext);
        free(videos[i]);
    }
    free(videos);

    return 0;
}
