//
// Created by Infinity on 2019-05-25.
//

#ifndef MEDIAUTIL_COMMON_H
#define MEDIAUTIL_COMMON_H

#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <cstdio>

#ifdef ASSET_DIR
#define ASSET(name) ASSET_DIR name
#else
#define ASSET(name) nullptr
#endif

#ifdef OUTPUT_DIR
#define OUTPUT(name) OUTPUT_DIR name
#else
#define OUTPUT(name)
#endif

#define exe(format, ...) { \
    char *command = new char[128]; \
    snprintf(command, 128, format, ## __VA_ARGS__); \
    system(command); \
    delete [] command; \
}

#endif //MEDIAUTIL_COMMON_H
