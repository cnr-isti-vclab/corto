cmake_minimum_required(VERSION 3.1)
project(corto)

SET(CORTO_SOURCE_PATH src)

SET(HEADERS
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

SET(SOURCES
	${CORTO_SOURCE_PATH}/bitstream.cpp
	${CORTO_SOURCE_PATH}/color_attribute.cpp
	${CORTO_SOURCE_PATH}/cstream.cpp
	${CORTO_SOURCE_PATH}/decoder.cpp
	${CORTO_SOURCE_PATH}/encoder.cpp
	${CORTO_SOURCE_PATH}/normal_attribute.cpp
	${CORTO_SOURCE_PATH}/tunstall.cpp)

ADD_LIBRARY(corto STATIC ${SOURCES} ${HEADERS})

SET(CORTO_DEFINITIONS "") 

if(MSVC)
	SET(CORTO_CFLAGS /nologo /W3)
	LIST(APPEND CORTO_DEFINITIONS _CRT_SECURE_NO_DEPRECATE)
else()
	SET(CORTO_CFLAGS -O2 -W -Wall -c -std=c++11 -pedantic)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	LIST(APPEND CORTO_CFLAGS -fPIC)
elseif(APPLE)
	LIST(APPEND CORTO_CFLAGS -fvisibility=hidden)
	if(IOS)
		LIST(APPEND CORTO_CFLAGS -fembed-bitcode)
		set_xcode_property(corto IPHONEOS_DEPLOYMENT_TARGET "9.2")
	endif(IOS)
endif()

target_include_directories(corto PUBLIC ${CORTO_SOURCE_PATH})
target_compile_definitions(corto PUBLIC ${CORTO_DEFINITIONS})
target_compile_options    (corto PUBLIC ${CORTO_CFLAGS})
set_target_properties     (corto PROPERTIES DEBUG_POSTFIX "d")

INSTALL(TARGETS corto
	RUNTIME DESTINATION bin
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
