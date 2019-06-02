
#include <cstdio>
#include <zconf.h>
#include "mediautil/clip.h"
#include "common.h"

int main(int argc, char *argv[]) {

    const char *video = ASSET("video.mp4");
    const char *output = OUTPUT("video_clip.mp4");

    int ret = clip(output, video, 2, 6, [](int progress) -> void {
        printf("progress: %d\n", progress);
    });

    if (ret == 0) {
        PLAY(output)
    }

    return ret;
}