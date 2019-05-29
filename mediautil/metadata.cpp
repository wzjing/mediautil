
#include "metadata.h"

int getMeta(Mp4Meta **meta, const char *filename) {
    AVFormatContext *formatContext = nullptr;
    int ret = 0;

    if (*meta == nullptr) {
        *meta = (Mp4Meta *) malloc(sizeof(Mp4Meta));
        (*meta)->formatMeta = nullptr;
        (*meta)->audioMeta = nullptr;
        (*meta)->videoMeta = nullptr;
    }

    ret = avformat_open_input(&formatContext, filename, nullptr, nullptr);
    if (ret < 0) return ret;

    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) return ret;

    av_dict_copy(&(*meta)->formatMeta, formatContext->metadata, 0);

    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            av_dict_copy(&(*meta)->audioMeta, formatContext->streams[i]->metadata, 0);
        } else if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            av_dict_copy(&(*meta)->videoMeta, formatContext->streams[i]->metadata, 0);
        }
    }

    avformat_free_context(formatContext);

    return 0;
}
