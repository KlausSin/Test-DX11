file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(PackLib STATIC ${FILE_SOURCES})

target_link_libraries(PackLib PRIVATE
    ${EXTERN_LIBS}
)

GroupSourcesByFolder(PackLib)
