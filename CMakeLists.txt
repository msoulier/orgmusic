# 1: Set the minimum required version of CMake to be 3.10
cmake_minimum_required(VERSION 3.10)

project(OrgMusic
    VERSION 1.0.0
    DESCRIPTION "Mike's music manager"
    HOMEPAGE_URL https://github.com/msoulier/orgmusic)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h @ONLY)

add_executable(orgmusic
    orgmusic.cpp)
target_link_libraries(orgmusic PUBLIC mediainfo)
target_link_libraries(orgmusic PUBLIC mikecpp)
target_include_directories(orgmusic PUBLIC $HOME/mediainfo/include)
target_include_directories(orgmusic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mikelibcpp)
target_link_directories(orgmusic PUBLIC $HOME/mediainfo/lib)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mikelibcpp)
