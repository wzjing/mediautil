//
// Created by android1 on 2019/5/29.
//

#ifndef MEDIAUTIL_TRANSCODE_H
#define MEDIAUTIL_TRANSCODE_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavfilter/avfilter.h>
#include <libswresample/swresample.h>
}

struct Config {
  // video
  AVCodecID video_codec_id = AV_CODEC_ID_H264;
  int width = 0;
  int height = 0;
  AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
  int video_bitrate = 0;

  // audio
  AVCodecID audio_codec_id = AV_CODEC_ID_AAC;
  AVSampleFormat sample_fmt = AV_SAMPLE_FMT_NONE;
  int channel_layout = AV_CH_LAYOUT_STEREO;
  int sample_rate = 0;
  int audio_bitrate = 0;
};

int transcode_audio(const char *output_filename, const char *input_filename, AVSampleFormat sample_fmt,
                    int sample_rate, uint64_t channel_layout, uint64_t bitrate);

#endif //MEDIAUTIL_TRANSCODE_H
