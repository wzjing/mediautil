#include <cstdio>
#include "mediautil/concat.h"
#include "mediautil/metadata.h"
#include "mediautil/remux.h"
#include "common.h"

int main(int argc, char *argv[]) {

    const char *video = ASSET("video.mp4");

    std::string path = getPath();
    std::string ts_filename = path + "/video_concat.ts";
    std::string output_filename = path + "/video_concat.mp4";
    printf("input_filename: %s\n", output_filename.c_str());

    const char *inputs[]{video, video};
    const char *titles[]{"Video 1: this is the first video\nstart", "Video 2"};

    int ret = 0;

#ifdef ENCODE
    ret = concat_encode(output_filename.c_str(), inputs, titles, 2, 40, 2);
#else
    Mp4Meta *meta = nullptr;
    getMeta(&meta, video);
    ret = concat_no_encode(ts_filename.c_str(), inputs, titles, 2, 40, 2);
    if (ret == 0) {
        ret = remux(output_filename.c_str(), ts_filename.c_str(), meta);
        remove(ts_filename.c_str());
    }
#endif


    if (ret == 0) {
        return exec("ffplay -i %s", output_filename.c_str());
    } else {
        return -1;
    }
}
