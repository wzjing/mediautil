
#include "remux.h"

#include "log.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

int remux(const char *output_filename, const char *input_filename, Mp4Meta *meta, ProgressCallback callback) {
    if (callback) callback(0);
    AVOutputFormat *outFormat = nullptr;
    AVFormatContext *inFmtContext = nullptr, *outFmtContext = nullptr;
    AVPacket pkt;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = nullptr;
    int stream_mapping_size = 0;
    int video_idx = 0;

    if ((ret = avformat_open_input(&inFmtContext, input_filename, nullptr, nullptr)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", input_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(inFmtContext, nullptr)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(inFmtContext, 0, input_filename, 0);

    avformat_alloc_output_context2(&outFmtContext, nullptr, nullptr, output_filename);
    if (!outFmtContext) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    stream_mapping_size = inFmtContext->nb_streams;
    stream_mapping = (int *) av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    outFormat = outFmtContext->oformat;

    for (i = 0; i < inFmtContext->nb_streams; i++) {
        AVStream *out_stream;
        AVStream *in_stream = inFmtContext->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(outFmtContext, nullptr);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
        if (meta && meta->audioMeta) {
            if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                av_dict_copy(&out_stream->metadata, meta->audioMeta, 0);
            } else if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                av_dict_copy(&out_stream->metadata, meta->videoMeta, 0);
                video_idx = i;
            }
        }
    }

    // copy metadata if exist
    if (meta) {
        if (meta->formatMeta) {
            av_dict_copy(&outFmtContext->metadata, meta->formatMeta, 0);
        }
    }

    av_dump_format(outFmtContext, 0, output_filename, 1);

    if (!(outFormat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outFmtContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", output_filename);
            goto end;
        }
    }

    ret = avformat_write_header(outFmtContext, nullptr);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }

    while (true) {
        AVStream *in_stream, *out_stream;

        ret = av_read_frame(inFmtContext, &pkt);
        if (ret < 0) {
            break;
        }

        if (callback && pkt.stream_index == video_idx) {
            int progress = 100 * pkt.pts/ inFmtContext->streams[video_idx]->duration;
            if (progress > 100) progress = 99;
            if (progress < 0) progress = 0;
            callback(progress);
        }

        in_stream = inFmtContext->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size ||
            stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = outFmtContext->streams[pkt.stream_index];
        logPacket(&pkt, &in_stream->time_base, "in");

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding) (AV_ROUND_DOWN | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding) (AV_ROUND_DOWN | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        logPacket(&pkt, &out_stream->time_base, "out");

        ret = av_interleaved_write_frame(outFmtContext, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error write packet: %s\n", av_err2str(ret));
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(outFmtContext);
    end:

    avformat_close_input(&inFmtContext);

    /* close output */
    if (outFmtContext && !(outFormat->flags & AVFMT_NOFILE))
        avio_closep(&outFmtContext->pb);
    avformat_free_context(outFmtContext);

    av_freep(&stream_mapping);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }
    if (callback) callback(100);
    return 0;
}
