
set(MEDIAUTIL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(MEDIAUTIL_LIB_DIR ${CMAKE_SOURCE_DIR}/lib)

include(FindPkgConfig)

add_definitions(-DDEBUG -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS)

if (${PKG_CONFIG_FOUND})
    message(STATUS "found pkg-config ${PKG_CONFIG_VERSION_STRING} in ${PKG_CONFIG_EXECUTABLE}")
else ()
    message(FATAL_ERROR "no pkg-config found, please install pkg-config and add libraries")
endif ()

pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libavformat libavfilter libavdevice libswscale libswresample libpostproc x264)

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

set(HEADERS
        video_filter.h
        audio_filter.h
        bgm.h
        concat.h
        log.h
        remux.h
        metadata.h
        clip.h
        transcode.h
        foundation.h)
set(SOURCES
        video_filter.cpp
        audio_filter.cpp
        bgm.cpp
        concat.cpp
        log.cpp
        remux.cpp
        metadata.cpp
        clip.cpp
        transcode.cpp)

add_library(mediautil SHARED ${SOURCES})

target_link_libraries(mediautil ${FFMPEG_LIBRARIES})

# INSTALL(FILES ${HEADERS} DESTINATION ${MEDIAUTIL_INCLUDE_DIR}/mediautil)
# INSTALL(TARGETS mediautil LIBRARY DESTINATION ${MEDIAUTIL_LIB_DIR})