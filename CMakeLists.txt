# 1: Set the minimum required version of CMake to be 3.10
cmake_minimum_required(VERSION 3.10)

project(OrgMusic
    VERSION 1.0.0
    DESCRIPTION "Mike's music manager"
    HOMEPAGE_URL https://github.com/msoulier/orgmusic)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h @ONLY)

file(GLOB SRC_FILES "*.cpp")

add_executable(orgmusic
    orgmusic.cpp)
target_link_libraries(orgmusic mediainfo)
