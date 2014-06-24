add_library(emelf-lib SHARED
	edh.c
	emelf.c
)

set_target_properties(emelf-lib PROPERTIES
	OUTPUT_NAME "emelf"
	SOVERSION ${EMELF_VERSION_MAJOR}
	PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/emelf.h"
)

install(TARGETS emelf-lib
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

add_executable(emelfread
	emelfread.c
)

target_link_libraries(emelfread emelf-lib)

install(TARGETS emelfread
	RUNTIME DESTINATION bin
)

# vim: tabstop=4
