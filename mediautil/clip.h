//
// Created by android1 on 2019/5/28.
//

#ifndef MEDIAUTIL_CLIP_H
#define MEDIAUTIL_CLIP_H

#include "foundation.h"

int clip(const char * output_filename, const char * input_filename, float from, float to, ProgressCallback callback);

#endif //MEDIAUTIL_CLIP_H
