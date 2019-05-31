#include <cstdio>
#include <string>
#include "mediautil/bgm.h"
#include "common.h"

int main(int argc, char *argv[]) {

    const char *video = ASSET("video.mp4");
    const char *audio = ASSET("bgm.mp3");

    const char *output = OUTPUT("video_bgm.mp4");

    int ret = add_bgm(output, video, audio, 1.6);

    if (ret == 0) {
        PLAY(output)
    }
    return ret;
}

