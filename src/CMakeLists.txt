add_library(emelf-lib SHARED
	edh.c
	emelf.c
)

set_target_properties(emelf-lib PROPERTIES
	OUTPUT_NAME "emelf"
	SOVERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}
	PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/emelf.h"
)

install(TARGETS emelf-lib
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

add_executable(emelfread
	emelfread.c
)

target_link_libraries(emelfread emelf-lib)

install(TARGETS emelfread
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# vim: tabstop=4
