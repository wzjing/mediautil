//
// Created by android1 on 2019/5/27.
//

#ifndef MEDIAUTIL_METADATA_H
#define MEDIAUTIL_METADATA_H

extern "C" {
#include <libavutil/dict.h>
#include <libavformat/avformat.h>
}

struct Mp4Meta {
  AVDictionary *formatMeta = nullptr;
  AVDictionary *videoMeta = nullptr;
  AVDictionary *audioMeta = nullptr;
};

int getMeta(Mp4Meta** meta, const char* filename);

#endif //MEDIAUTIL_METADATA_H
