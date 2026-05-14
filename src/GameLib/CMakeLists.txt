file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(GameLib STATIC ${FILE_SOURCES})

target_link_libraries(GameLib PRIVATE
    ${EXTERN_LIBS}
)

GroupSourcesByFolder(GameLib)
