link_libraries(mediautil)

set(TEST_BASE common.h common.cpp)

add_executable(concat concat.cpp ${TEST_BASE})
add_executable(bgm bgm.cpp ${TEST_BASE})
add_executable(clip clip.cpp ${TEST_BASE})

target_link_libraries(concat mediautil)
target_link_libraries(bgm mediautil)
target_link_libraries(clip mediautil)