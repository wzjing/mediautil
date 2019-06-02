#include <cstdio>
#include <string>
#include "mediautil/bgm.h"
#include "common.h"

int main(int argc, char *argv[]) {

    const char *video = ASSET("movie.mp4");
    const char *audio = ASSET("bgm.mp3");

    const char *output = OUTPUT("video_bgm.mp4");

    int ret = add_bgm(output, video, audio, 0.8, [](int progress) -> void {
        printf("progress: %d\n", progress);
    });

    if (ret == 0) {
        PLAY(output)
    }
    return ret;
}