//
// Created by android1 on 2019/5/29.
//

#include "transcode.h"
#include "log.h"
#include "foundation.h"
#include "audio_filter.h"

#define TAG "transcode"

int transcode_audio(const char *output_filename, const char *input_filename, AVSampleFormat sample_fmt,
                    int sample_rate, uint64_t channel_layout, uint64_t bitrate) {
    AVFormatContext *inFmtCtx = nullptr;
    AVFormatContext *outFmtCtx = nullptr;

    AVCodecContext *aDecCtx = nullptr;
    AVCodecContext *aEncCtx = nullptr;

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

            AVCodec *encoder = avcodec_find_encoder(outFmtCtx->oformat->audio_codec);
            nonnull(encoder, "unable to find audio encoder")
            aOutStream = avformat_new_stream(outFmtCtx, encoder);
            nonnull(aOutStream, "unable to create output audio stream")
            aOutStream->id = outFmtCtx->nb_streams - 1;
            aEncCtx = avcodec_alloc_context3(encoder);
            nonnull(aOutStream, "unable to create output audio stream")
            aEncCtx->codec_id = encoder->id;
            aEncCtx->sample_fmt = sample_fmt ? sample_fmt : aDecCtx->sample_fmt;
            aEncCtx->sample_rate = sample_rate ? sample_rate : aDecCtx->sample_rate;
            aEncCtx->channel_layout = channel_layout;
            aEncCtx->channels = av_get_channel_layout_nb_channels(channel_layout);
            aEncCtx->bit_rate = bitrate ? bitrate : aDecCtx->bit_rate;
            aEncCtx->time_base = (AVRational) {1, aEncCtx->sample_rate};
            aOutStream->time_base = aEncCtx->time_base;
            if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
                aEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            ret = avcodec_open2(aEncCtx, encoder, nullptr);
            checkret(ret, "unable to create output audio encode context")
            ret = avcodec_parameters_from_context(aOutStream->codecpar, aEncCtx);
            checkret(ret, "unable to copy parameter from audio context to stream")
            av_dict_copy(&aOutStream->metadata, inStream->metadata, 0);

            logStream(inStream, "IN", 0);
            logStream(aOutStream, "OUT", 0);
            break;
        }
    }

    logContext(aDecCtx, "IN", 0);
    logContext(aEncCtx, "OUT", 0);

    if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        LOGD(TAG, "Opening file: %s\n", output_filename);
        ret = avio_open(&outFmtCtx->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE(TAG, "could not open %s (%s)\n", output_filename, av_err2str(ret));
            return -1;
        }
    }

    ret = avformat_write_header(outFmtCtx, nullptr);
    if (ret < 0) {
        LOGE(TAG, "unable to open output file header: %s\n", av_err2str(ret));
        return -1;
    }

    AVFrame *inAudioFrame = av_frame_alloc();
    AVFrame *outAudioFrame = av_frame_alloc();

    outAudioFrame->format = aEncCtx->sample_fmt;
    outAudioFrame->sample_rate = aEncCtx->sample_rate;
    outAudioFrame->channel_layout = aEncCtx->channel_layout;
    outAudioFrame->nb_samples = aEncCtx->frame_size;
    ret = av_frame_get_buffer(outAudioFrame, 0);
    checkret(ret, "unable to get out frame buffer")

    int64_t audio_pts = 0;

    // filter
    AudioFilter filter;
    char description[512];
    AudioConfig inConfig(aDecCtx->sample_fmt, aDecCtx->sample_rate, aDecCtx->channel_layout, aDecCtx->time_base);
    AudioConfig outConfig(aEncCtx->sample_fmt, aEncCtx->sample_rate, aEncCtx->channel_layout, aEncCtx->time_base);
    char ch_layout[64];
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout),
                                 av_get_channel_layout_nb_channels(aEncCtx->channel_layout), aEncCtx->channel_layout);
    snprintf(description, sizeof(description),
             "[in]aresample=sample_rate=%d[res];[res]aformat=sample_fmts=%s:sample_rates=%d:channel_layouts=%s[out]",
             aEncCtx->sample_rate,
             av_get_sample_fmt_name(aEncCtx->sample_fmt),
             aEncCtx->sample_rate,
             ch_layout);
    filter.create(description, &inConfig, &outConfig);
    filter.dumpGraph();

    while (true) {
        AVPacket inPacket{nullptr};
        av_init_packet(&inPacket);
        ret = av_read_frame(inFmtCtx, &inPacket);
        if (ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOGE(TAG, "error while read input packet: %s\n", av_err2str(ret));
            return -1;
        }

        if (inPacket.stream_index == aOutStream->index) {
            logPacket(&inPacket, &inFmtCtx->streams[inPacket.stream_index]->time_base, "IN");
            ret = avcodec_send_packet(aDecCtx, &inPacket);
            if (ret != 0) {
                LOGW(TAG, "unable to send packet: %s\n", av_err2str(ret));
            }
            ret = avcodec_receive_frame(aDecCtx, inAudioFrame);

            if (ret == 0) {
                logFrame(inAudioFrame, &inFmtCtx->streams[inPacket.stream_index]->time_base, "IN", 0);
                ret = filter.addInput1(inAudioFrame);
                av_frame_unref(inAudioFrame);
                if (ret < 0) {
                    LOGW(TAG, "unable to add filter audio frame: %s\n", av_err2str(ret));
                }

                do {
                    outAudioFrame->nb_samples = aEncCtx->frame_size;
                    ret = filter.getFrame(outAudioFrame);
                    if (ret == 0) {

                        outAudioFrame->pts = audio_pts;
                        audio_pts += outAudioFrame->nb_samples;

                        logFrame(outAudioFrame, &aEncCtx->time_base, "OUT", 0);

                        ret = avcodec_send_frame(aEncCtx, outAudioFrame);
                        if (ret < 0) {
                            LOGW(TAG, "unable to send frame: %s\n", av_err2str(ret));
                        }
                    } else {
                        LOGW(TAG, "unable to get filter audio frame: %s\n", av_err2str(ret));
                        break;
                    }

                    do {
                        AVPacket outPacket{nullptr};
                        av_init_packet(&outPacket);
                        ret = avcodec_receive_packet(aEncCtx, &outPacket);
                        if (ret == 0) {
                            av_packet_rescale_ts(&outPacket, aEncCtx->time_base, aOutStream->time_base);
                            outPacket.stream_index = aOutStream->index;
                            logPacket(&outPacket, &aOutStream->time_base, "OUT");
                            ret = av_interleaved_write_frame(outFmtCtx, &outPacket);
                            if (ret < 0) {
                                LOGE(TAG, "unable to write packet: %s\n", av_err2str(ret));
                                break;
                            }
                        } else {
                            LOGW(TAG, "unable to receive packet: %s\n", av_err2str(ret));
                            break;
                        }
                    } while (true);

                } while (true);

                LOGD(TAG, "\n");

            } else {
                LOGW(TAG, "unable to receive frame: %s\n", av_err2str(ret));
            }
        }
    }

    // flush
    int eof = 0;
    do {
        ret = filter.getFrame(outAudioFrame);
        if (ret == 0) {
            outAudioFrame->pts = audio_pts;
            audio_pts += outAudioFrame->nb_samples;
            logFrame(outAudioFrame, &aEncCtx->time_base, "FLUSH", 0);
        } else {
            LOGD(TAG, "filter queue finished\n");
        }

        ret = avcodec_send_frame(aEncCtx, ret == 0 ? outAudioFrame : nullptr);
        if (ret < 0) {
            LOGW(TAG, "unable to send frame: %s\n", av_err2str(ret));
        }
        do {
            AVPacket outPacket{nullptr};
            ret = avcodec_receive_packet(aEncCtx, &outPacket);
            if (ret == 0) {
                av_packet_rescale_ts(&outPacket, aEncCtx->time_base, aOutStream->time_base);
                outPacket.stream_index = aOutStream->index;
                logPacket(&outPacket, &aOutStream->time_base, "FLUSH");
                ret = av_interleaved_write_frame(outFmtCtx, &outPacket);
                if (ret < 0) {
                    LOGE(TAG, "unable to write packet: %s\n", av_err2str(ret));
                    eof = 1;
                    break;
                }
            } else if (ret == AVERROR_EOF) {
                LOGW(TAG, "reach end\n");
                eof = 1;
                break;
            } else {
                LOGE(TAG, "unable to receive packet: %s\n", av_err2str(ret));
                break;
            }
        } while (true);

    } while (!eof);

    filter.destroy();

    av_write_trailer(outFmtCtx);

    avformat_close_input(&inFmtCtx);

    av_frame_free(&inAudioFrame);
    av_frame_free(&outAudioFrame);

    avcodec_free_context(&aDecCtx);
    avcodec_free_context(&aEncCtx);

    avformat_free_context(inFmtCtx);
    avformat_free_context(outFmtCtx);

    return 0;
}
