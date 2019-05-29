
#include <cstdio>
#include "mediautil/clip.h"
#include "common.h"

int main(int argc, char *argv[]) {

    const char *video = ASSET("video.mp4");

    std::string path = getPath();
    std::string output_filename = path + "/video_clip.mp4";
    printf("output_filename: %s\n", output_filename.c_str());

    clip(output_filename.c_str(), video, 2, 6);
}