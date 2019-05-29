link_libraries(mediautil)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory -fPIE -pie -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -O1")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fPIE -pie -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -O1")

set(TEST_BASE common.h common.cpp)

add_executable(concat concat.cpp ${TEST_BASE})
add_executable(bgm bgm.cpp ${TEST_BASE})
add_executable(clip clip.cpp ${TEST_BASE})

include_directories(${MEDIAUTIL_INCLUDE_DIR})
link_directories(${MEDIAUTIL_LIB_DIR})

target_link_libraries(concat mediautil)
target_link_libraries(bgm mediautil)