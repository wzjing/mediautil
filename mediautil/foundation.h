//
// Created by android1 on 2019/5/30.
//

#ifndef MEDIAUTIL_FOUNDATION_H
#define MEDIAUTIL_FOUNDATION_H

#include <functional>

#define checkret(ret, msg, ...) if(ret < 0) {LOGE(TAG, msg ": %s\n", ##__VA_ARGS__, av_err2str(ret));return -1;}
#define nonnull(ptr, msg) if(!ptr){LOGD(TAG, "null pointer: %s\n", msg);return -1;}

typedef const std::function<void(int progress)> &ProgressCallback;

#endif //MEDIAUTIL_FOUNDATION_H
