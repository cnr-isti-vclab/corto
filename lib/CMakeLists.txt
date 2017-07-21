set(CORTOLIB_CFLAGS ${CORTO_CFLAGS})

set(LIB_HEADERS
	${CORTO_SOURCE_PATH}/bitstream.h
	${CORTO_SOURCE_PATH}/color_attribute.h
	${CORTO_SOURCE_PATH}/corto.h
	${CORTO_SOURCE_PATH}/cstream.h
	${CORTO_SOURCE_PATH}/decoder.h
	${CORTO_SOURCE_PATH}/encoder.h
	${CORTO_SOURCE_PATH}/index_attribute.h
	${CORTO_SOURCE_PATH}/normal_attribute.h
	${CORTO_SOURCE_PATH}/point.h
	${CORTO_SOURCE_PATH}/tunstall.h
	${CORTO_SOURCE_PATH}/vertex_attribute.h
	${CORTO_SOURCE_PATH}/zpoint.h)

set(LIB_SOURCES
	${CORTO_SOURCE_PATH}/bitstream.cpp
	${CORTO_SOURCE_PATH}/color_attribute.cpp
	${CORTO_SOURCE_PATH}/cstream.cpp
	${CORTO_SOURCE_PATH}/decoder.cpp
	${CORTO_SOURCE_PATH}/encoder.cpp
	${CORTO_SOURCE_PATH}/normal_attribute.cpp
	${CORTO_SOURCE_PATH}/tunstall.cpp)


if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	LIST(APPEND CORTOLIB_CFLAGS -fPIC)
elseif(APPLE)
	LIST(APPEND CORTOLIB_CFLAGS -fvisibility=hidden)
	if(IOS)
		LIST(APPEND CORTOLIB_CFLAGS -fembed-bitcode)
		set_xcode_property(corto IPHONEOS_DEPLOYMENT_TARGET "9.2")
		set_xcode_property(cortoexe IPHONEOS_DEPLOYMENT_TARGET "9.2")
	endif(IOS)
endif()

target_include_directories(cortolib PUBLIC ${CORTO_SOURCE_PATH})
target_compile_definitions(cortolib PUBLIC ${CORTO_DEFINITIONS})
target_compile_options    (cortolib PUBLIC ${CORTOLIB_CFLAGS})
set_target_properties     (cortolib PROPERTIES DEBUG_POSTFIX "d")
set_target_properties     (cortolib PROPERTIES OUTPUT_NAME "corto")


add_library(cortolib OBJECT ${LIB_SOURCES} ${LIB_HEADERS})
set_property(TARGET ${cortolib} PROPERTY POSITION_INDEPENDENT_CODE 1)

# shared and static libraries built from the same object files
add_library(corto_shared SHARED $<TARGET_OBJECTS:cortolib>)
add_library(corto_static STATIC $<TARGET_OBJECTS:cortolib>)

INSTALL(TARGETS corto_shared corto_static
	EXPORT CortoTargets
	LIBRARY DESTINATION ${INSTALL_LIB_DIR}" COMPONENT shlib
	LIBRARY DESTINATION ${INSTALL_LIB_DIR}" COMPONENT lib
	PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/corto"
	COMPONENT dev
)

