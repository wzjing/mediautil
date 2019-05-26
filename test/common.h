//
// Created by Infinity on 2019-05-25.
//

#ifndef MEDIAUTIL_COMMON_H
#define MEDIAUTIL_COMMON_H

#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <cstdio>

#ifndef VIDEO_SOURCE
#define VIDEO_SOURCE nullptr
#endif

#ifndef AUDIO_SOURCE
#define AUDIO_SOURCE nullptr
#endif

std::string getPath();

int exec(const char *format, ...) __attribute__((__format__ (__printf__, 1, 2)));

#endif //MEDIAUTIL_COMMON_H
