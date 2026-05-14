file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(qMin32Lib STATIC ${FILE_SOURCES})

target_link_libraries(qMin32Lib PRIVATE
    ${EXTERN_LIBS}
)

GroupSourcesByFolder(qMin32Lib)
