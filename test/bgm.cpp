#include <cstdio>
#include <string>
#include <mediautil/bgm.h>
#include "common.h"

int main(int argc, char *argv[]) {

    if (VIDEO_SOURCE && AUDIO_SOURCE) {
        std::string path = getPath();
        path += "/video_bgm.mp4";
        printf("path: %s\n", path.c_str());

        int ret = add_bgm(path.c_str(), VIDEO_SOURCE, AUDIO_SOURCE, 1.6);

        if (ret == 0) {
            return exec("ffplay -i %s", path.c_str());
        } else {
            return -1;
        }

    } else {
        printf("input video or audio error\n");
        return -1;
    }
}

