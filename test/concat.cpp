#include <cstdio>
#include "mediautil/concat.h"
#include "mediautil/metadata.h"
#include "mediautil/remux.h"
#include "common.h"

//#define ENCODE

int main(int argc, char *argv[]) {

    const char *input = ASSET("video.mp4");

    const char *ts_filename = OUTPUT("concat.ts");
    const char *output = OUTPUT("concat.mp4");

    const char *inputs[]{input, input};
    const char *titles[]{"Video 1: this is the first video\nstart", "Video 2"};

    int ret = 0;

#ifdef ENCODE
    ret = concat_encode(output, inputs, titles, 2, 40, 2, [](int progress) -> void {
        printf("progress: %d\n", progress);
    });
#else
    Mp4Meta *meta = nullptr;
    getMeta(&meta, input);
    ret = concat_no_encode(ts_filename, inputs, titles, 2, 40, 2, [](int progress) -> void {
        printf("progress: %d\n", progress);
    });
    if (ret == 0) {
        ret = remux(output, ts_filename, meta, [](int progress) -> void {
            printf("progress: %d\n", progress);
        });
        remove(ts_filename);
    }
#endif

    if (ret == 0) {
        PLAY(output)
    } else  {
        printf("failed: %d\n", ret);
    }

    return ret;
}
