
#include <cstdio>
#include <mediautil/clip.h>
#include "common.h"

int main(int argc, char *argv[]) {
    if (VIDEO_SOURCE && AUDIO_SOURCE) {
        std::string path = getPath();
        std::string output_filename = path + "/video_clip.mp4";
        printf("output_filename: %s\n", output_filename.c_str());

       clip(output_filename.c_str(), VIDEO_SOURCE, 2, 6);

    } else {
        printf("error\n");
        return -1;
    }
}