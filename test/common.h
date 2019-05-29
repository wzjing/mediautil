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


std::string getPath();

int exec(const char *format, ...) __attribute__((__format__ (__printf__, 1, 2)));

#endif //MEDIAUTIL_COMMON_H
