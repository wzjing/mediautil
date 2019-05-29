cmake_minimum_required(VERSION 3.10)
project(mediautil)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_definitions("-DASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/\"" "-DOUTPUT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/output/\"")

include(FindPkgConfig)

add_definitions(-DDEBUG -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS)

if (${PKG_CONFIG_FOUND})
    message(STATUS "found pkg-config ${PKG_CONFIG_VERSION_STRING} in ${PKG_CONFIG_EXECUTABLE}")
else ()
    message(FATAL_ERROR "no pkg-config found, please install pkg-config and add libraries")
endif ()

pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libavformat libavfilter libswscale libswresample x264 freetype2)

if (FFMPEG_FOUND)
    foreach (cflag ${FFMPEG_CFLAGS})
        message(STATUS "cflags: ${cflag}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cflag}")
    endforeach (cflag)
else ()
    message(FATAL_ERROR "no ffmpeg")
endif ()

link_directories(${FFMPEG_LIBRARY_DIRS})

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(mediautil)

add_subdirectory(test)