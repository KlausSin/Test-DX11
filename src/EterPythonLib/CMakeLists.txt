file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_library(EterPythonLib STATIC ${FILE_SOURCES})

target_link_libraries(EterPythonLib PRIVATE
    ${EXTERN_LIBS}
)

GroupSourcesByFolder(EterPythonLib)
