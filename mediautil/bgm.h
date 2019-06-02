
#ifndef VIDEOBOX_MIX_BGM_H
#define VIDEOBOX_MIX_BGM_H

#include "foundation.h"

int add_bgm(const char *output_filename, const char *input_filename, const char *bgm_filename,
            float bgm_volume, ProgressCallback callback);

#endif //VIDEOBOX_MIX_BGM_H
