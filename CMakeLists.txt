cmake_minimum_required(VERSION 2.8.4)
project(emelf C)

set(EMELF_VERSION_MAJOR 0)
set(EMELF_VERSION_MINOR 1)
set(EMELF_VERSION_PATCH 0)
set(EMELF_VERSION ${EMELF_VERSION_MAJOR}.${EMELF_VERSION_MINOR}.${EMELF_VERSION_PATCH})

include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions(-Wall -std=c99 -D_XOPEN_SOURCE=700 -O2 -DEMELF_VERSION="${EMELF_VERSION}")

configure_file(
	emelf-config.cmake.in
	${PROJECT_BINARY_DIR}/emelf-config.cmake
	@ONLY
)

configure_file(
	emelf-config-version.cmake.in
	${PROJECT_BINARY_DIR}/emelf-config-version.cmake
	@ONLY
)

install(FILES
	${PROJECT_BINARY_DIR}/emelf-config.cmake
	${PROJECT_BINARY_DIR}/emelf-config-version.cmake
	DESTINATION lib/emelf
	COMPONENT dev
)

add_subdirectory(src)

# vim: tabstop=4
