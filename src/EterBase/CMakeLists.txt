file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(EterBase STATIC ${FILE_SOURCES})

target_link_libraries(EterBase PRIVATE
    ${EXTERN_LIBS}
)

target_compile_definitions(EterBase PRIVATE
    SODIUM_STATIC
)

GroupSourcesByFolder(EterBase)
