link_libraries(mediautil)

set(TEST_BASE common.h)

add_definitions("-DASSET_DIR=\"${CMAKE_SOURCE_DIR}/assets/\""
        "-DOUTPUT_DIR=\"${CMAKE_SOURCE_DIR}/output/\"")


if (${CMAKE_SYSTEM} MATCHES "Linux.+Microsoft")
    add_definitions(-DWSL)
endif ()

find_program(player ffplay)

if (player)
    message(STATUS "found player: ${player}")
    add_definitions("-DPLAYER=\"${player} -i \"")
else ()
    message(STATUS "no player found")
endif ()

add_executable(concat concat.cpp ${TEST_BASE})
add_executable(bgm bgm.cpp ${TEST_BASE})
add_executable(clip clip.cpp ${TEST_BASE})
add_executable(transcode transcode.cpp ${TEST_BASE})

target_link_libraries(concat mediautil)
target_link_libraries(bgm mediautil)
target_link_libraries(clip mediautil)
target_link_libraries(transcode mediautil)