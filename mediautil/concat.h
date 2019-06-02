//
// Created by android1 on 2019/5/22.
//

#ifndef VIDEOBOX_CONCAT_ADD_TITLE_H
#define VIDEOBOX_CONCAT_ADD_TITLE_H

#include "metadata.h"
#include "foundation.h"

int concat_no_encode(const char *output_filename, const char **input_filenames, const char **titles, int nb_inputs,
                     int font_size, int title_duration, ProgressCallback callback);

int concat_encode(const char *output_filename, const char **input_filenames, const char **titles, int nb_inputs,
                     int font_size, int title_duration, ProgressCallback callback);

#endif //VIDEOBOX_CONCAT_ADD_TITLE_H
