link_libraries(mediautil)

set(CMAKE_CXX_STANDARD 11)

add_definitions("-DVIDEO_SOURCE=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/video.mp4\""
        "-DAUDIO_SOURCE=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/audio.aac\"")

set(TEST_BASE common.h common.cpp)

add_executable(concat concat.cpp ${TEST_BASE})
add_executable(bgm bgm.cpp ${TEST_BASE})

include_directories(${MEDIAUTIL_INCLUDE_DIR})
link_directories(${MEDIAUTIL_LIB_DIR})

target_link_libraries(concat mediautil)
target_link_libraries(bgm mediautil)