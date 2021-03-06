cmake_minimum_required(VERSION 2.8.4)
project(emelf C)

include(GNUInstallDirs)
include(appversion.cmake)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions(-Wall -std=c99 -D_XOPEN_SOURCE=700 -O2 -DEMELF_VERSION="${APP_VERSION}")

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
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/emelf
	COMPONENT dev
)

add_subdirectory(src)

# vim: tabstop=4
