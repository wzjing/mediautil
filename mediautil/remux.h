//
// Created by android1 on 2019/5/27.
//

#ifndef MEDIAUTIL_REMUX_H
#define MEDIAUTIL_REMUX_H

#include "metadata.h"
#include "foundation.h"

int remux(const char * output_filename, const char * input_filename, Mp4Meta *meta, ProgressCallback callback);

#endif //MEDIAUTIL_REMUX_H
