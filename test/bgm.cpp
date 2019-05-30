#include <cstdio>
#include <string>
#include "mediautil/bgm.h"
#include "common.h"

int main(int argc, char *argv[]) {

    const char *video = ASSET("video.mp4");
    const char *audio = ASSET("bgm.aac");

    const char *output = OUTPUT("video_bgm.mp4");

    int ret = add_bgm(output, video, audio, 1.6);


#ifdef PLAYER
    if (ret == 0) {
        exe("ffplay -i %s", output)
    }
#endif
    return ret;
}

