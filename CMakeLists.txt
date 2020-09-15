cmake_minimum_required(VERSION 3.1)
project(corto)

SET(CORTO_SOURCE_PATH src)

SET(LIB_HEADERS
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
	${CORTO_SOURCE_PATH}/zpoint.h
	${CORTO_SOURCE_PATH}/corto_codec.h)

SET(LIB_SOURCES
	${CORTO_SOURCE_PATH}/bitstream.cpp
	${CORTO_SOURCE_PATH}/color_attribute.cpp
	${CORTO_SOURCE_PATH}/cstream.cpp
	${CORTO_SOURCE_PATH}/decoder.cpp
	${CORTO_SOURCE_PATH}/encoder.cpp
	${CORTO_SOURCE_PATH}/normal_attribute.cpp
	${CORTO_SOURCE_PATH}/tunstall.cpp
	${CORTO_SOURCE_PATH}/corto_codec.cpp)

SET(EXE_SOURCE
	${CORTO_SOURCE_PATH}/main.cpp
	${CORTO_SOURCE_PATH}/meshloader.cpp
	${CORTO_SOURCE_PATH}/tinyply.cpp)

SET(EXE_HEADERS
	${CORTO_SOURCE_PATH}/meshloader.h
	${CORTO_SOURCE_PATH}/tinyply.h
	${CORTO_SOURCE_PATH}/objload.h)

ADD_LIBRARY(cortolib STATIC ${LIB_SOURCES} ${LIB_HEADERS})
ADD_LIBRARY(cortocodec_unity SHARED ${LIB_SOURCES} ${LIB_HEADERS})
ADD_EXECUTABLE(cortoexe ${EXE_SOURCE} ${EXE_HEADERS})

SET(CORTO_DEFINITIONS "") 

if(MSVC)
	SET(CORTO_CFLAGS /nologo /W3)
	LIST(APPEND CORTO_DEFINITIONS _CRT_SECURE_NO_DEPRECATE)
else()
	SET(CORTO_CFLAGS -O2 -W -Wall -c -std=c++11 -pedantic)
endif()

SET(CORTOEXE_CFLAGS ${CORTO_CFLAGS})
SET(CORTOLIB_CFLAGS ${CORTO_CFLAGS})

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	LIST(APPEND CORTOLIB_CFLAGS -fPIC)
elseif(APPLE)
	LIST(APPEND CORTOLIB_CFLAGS -fvisibility=hidden)
	if(IOS)
		LIST(APPEND CORTOLIB_CFLAGS -fembed-bitcode)
		set_xcode_property(cortolib IPHONEOS_DEPLOYMENT_TARGET "9.2")
		set_xcode_property(cortoexe IPHONEOS_DEPLOYMENT_TARGET "9.2")
	endif(IOS)
endif()

target_include_directories(cortolib PUBLIC ${CORTO_SOURCE_PATH})
target_include_directories(cortocodec_unity PUBLIC ${CORTO_SOURCE_PATH})
target_include_directories(cortoexe PUBLIC ${CORTO_SOURCE_PATH})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    # G++
    target_compile_options(cortocodec_unity PRIVATE -Wall -Wextra)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    # MSVC
    target_compile_options(cortocodec_unity PRIVATE /EHsc /MTd /W2 /c)
    # Set the DLLEXPORT variable to export symbols
    target_compile_definitions(cortocodec_unity PRIVATE WIN_EXPORT)
endif()

target_link_libraries(cortoexe PUBLIC cortolib)

target_compile_definitions(cortolib PUBLIC ${CORTO_DEFINITIONS})
target_compile_options    (cortolib PUBLIC ${CORTOLIB_CFLAGS})
target_compile_options    (cortoexe PUBLIC ${CORTOEXE_CFLAGS})
set_target_properties     (cortolib PROPERTIES DEBUG_POSTFIX "d")
set_target_properties     (cortoexe PROPERTIES OUTPUT_NAME "corto")
set_target_properties     (cortolib PROPERTIES OUTPUT_NAME "corto")

INSTALL(TARGETS cortolib cortoexe
	RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

INSTALL(FILES
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
	${CORTO_SOURCE_PATH}/zpoint.h
	DESTINATION include/corto)
