//
// Created by android1 on 2019/5/30.
//

#include "common.h"
#include "mediautil/transcode.h"

int main(int argc, char *argv[]) {
    const char *input = ASSET("bgm.mp3");
    const char *output = OUTPUT("transcode.aac");

    int ret = transcode_audio(output, input, AV_SAMPLE_FMT_FLTP, 44100, AV_CH_LAYOUT_STEREO, 135000);

#ifdef PLAYER
    if (ret == 0) {
        exe("ffplay -i %s", output);
    }
#endif

    return ret;

}

