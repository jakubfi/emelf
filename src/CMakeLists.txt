add_library(emelf SHARED
	edh.c
	emelf.c
)

ADD_EXECUTABLE(emelfread
emelfread.c
)

TARGET_LINK_LIBRARIES(emelfread emelf)

set_target_properties(emelf PROPERTIES
	SOVERSION ${EMELF_VERSION_MAJOR}
	PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/include/emelf.h"
)

install(TARGETS emelfread
	RUNTIME DESTINATION bin
)

install(TARGETS emelf
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

# vim: tabstop=4
