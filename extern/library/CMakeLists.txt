add_library(DirectXTexExternal STATIC IMPORTED)
set_target_properties(DirectXTexExternal PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/DirectXTex.lib"
)

add_library(JoltExternal STATIC IMPORTED)
set_target_properties(JoltExternal PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/Jolt.lib"
)

add_subdirectory(Granny)
add_subdirectory(Python)
add_subdirectory(SpeedTree)
add_subdirectory(WebView)
