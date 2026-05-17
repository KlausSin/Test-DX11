file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_executable(UserInterface WIN32 ${FILE_SOURCES} ${CMAKE_SOURCE_DIR}/src/UserInterface/UserInterface.rc)

set_target_properties(UserInterface PROPERTIES
	LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'"
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
	OUTPUT_NAME_DEBUG "Metin2_Debug"
	OUTPUT_NAME_RELEASE "Metin2_Release"
	OUTPUT_NAME_RELWITHDEBINFO "Metin2_RelWithDebInfo"
)

target_compile_definitions(UserInterface PRIVATE
    SODIUM_STATIC
)


target_link_libraries(UserInterface PRIVATE
	AudioLib
	Discord
	EffectLib
	EterBase
	EterGrnLib
	EterLib
	EterLocale
	EterPythonLib
	GameLib
	PRTerrainLib
	PythonModules
	ScriptLib
	SpeedTreeLib
	SphereLib
	PackLib
	qMin32Lib

	${EXTERN_LIBS}
	bcrypt
    pathcch

	d3d11
	dxgi
	d3dcompiler
	ws2_32
	version
	Dbghelp
)

target_compile_definitions(UserInterface PRIVATE
    JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
    JPH_USE_DX12
    JPH_USE_CPU_COMPUTE
    JPH_DEBUG_RENDERER
    JPH_PROFILE_ENABLED
    JPH_OBJECT_STREAM

    JPH_USE_AVX2
    JPH_USE_AVX
    JPH_USE_SSE4_1
    JPH_USE_SSE4_2
    JPH_USE_LZCNT
    JPH_USE_TZCNT
    JPH_USE_F16C
    JPH_USE_FMADD
)

GroupSourcesByFolder(UserInterface)

add_custom_command(TARGET UserInterface POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:UserInterface>" "${CMAKE_SOURCE_DIR}/../../Client/Metin2.exe"
	COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_PDB_FILE:UserInterface>" "${CMAKE_SOURCE_DIR}/../../Client/Metin2.pdb"
	COMMENT "Copying client binary to Client/"
)
