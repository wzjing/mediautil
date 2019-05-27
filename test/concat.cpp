#include <cstdio>
#include <mediautil/concat.h>
#include <mediautil/remux.h>
#include "common.h"

int main(int argc, char *argv[]) {
    if (VIDEO_SOURCE && AUDIO_SOURCE) {
        std::string path = getPath();
        std::string ts_filename = path + "/video_concat.ts";
        std::string output_filename = path + "/video_concat.mp4";
        printf("input_filename: %s\n", output_filename.c_str());

        const char *inputs[]{VIDEO_SOURCE, VIDEO_SOURCE};
        const char *titles[]{"Video 1: this is the first video\nstart", "Video 2"};

        Mp4Meta* meta = nullptr;

        getMeta(&meta, inputs[0]);

//        int ret = concat_add_title(ts_filename.c_str(), inputs, titles, 2, 40, 2);
        int ret = concat_encode(output_filename.c_str(), inputs, titles, 2, 40, 2);

        if (ret == 0) {

//            ret = remux(output_filename.c_str(), ts_filename.c_str(), meta);

            if (ret == 0) {
                remove(ts_filename.c_str());
                return exec("ffplay -i %s", output_filename.c_str());
            } else {
                return -1;
            }

        } else {
            return -1;
        }

    } else {
        printf("error\n");
        return -1;
    }
}
