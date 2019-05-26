#include <cstdio>
#include <mediautil/concat.h>
#include "common.h"

int main(int argc, char *argv[]) {
    if (VIDEO_SOURCE && AUDIO_SOURCE) {
        std::string path = getPath();
        path += "/video_concat.mp4";
        printf("path: %s\n", path.c_str());

        const char *inputs[]{VIDEO_SOURCE, VIDEO_SOURCE};
        const char *titles[]{"Video 1: this is the first video\nstart", "Video 2"};

        int ret = concat_add_title(path.c_str(), inputs, titles, 2, 40, 2);

        if (ret == 0) {
            return exec("ffplay -i %s", path.c_str());
        } else {
            return -1;
        }

    } else {
        printf("error\n");
        return -1;
    }
}
