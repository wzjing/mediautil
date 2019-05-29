#include <cstdio>
#include "mediautil/concat.h"
#include "mediautil/metadata.h"
#include "mediautil/remux.h"
#include "common.h"

int main(int argc, char *argv[]) {

    const char *video = ASSET("video.mp4");

    const char *ts_filename = OUTPUT("concat.ts");
    const char *output_filename = OUTPUT("concat.mp4");

    const char *inputs[]{video, video};
    const char *titles[]{"Video 1: this is the first video\nstart", "Video 2"};

    int ret = 0;

#ifdef ENCODE
    ret = concat_encode(output_filename, inputs, titles, 2, 40, 2);
#else
    Mp4Meta *meta = nullptr;
    getMeta(&meta, video);
    ret = concat_no_encode(ts_filename, inputs, titles, 2, 40, 2);
    if (ret == 0) {
        ret = remux(output_filename, ts_filename, meta);
        remove(ts_filename);
    }
#endif


    if (ret == 0) {
        return exec("ffplay -i %s", output_filename);
    } else {
        return -1;
    }
}
