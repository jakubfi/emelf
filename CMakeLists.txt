cmake_minimum_required(VERSION 2.8.4)
project(emelf C)

set(EMELF_VERSION_MAJOR 0)
set(EMELF_VERSION_MINOR 1)
set(EMELF_VERSION_PATCH 0)
set(EMELF_VERSION ${EMELF_VERSION_MAJOR}.${EMELF_VERSION_MINOR}.${EMELF_VERSION_PATCH})

include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions(-Wall -std=c99 -D_XOPEN_SOURCE=700 -O2 -DEMELF_VERSION="${EMELF_VERSION}" -g)

add_subdirectory(src build)

# vim: tabstop=4
