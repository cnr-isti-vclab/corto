cmake_minimum_required(VERSION 3.1)
project(corto)

SET(HEADERS
	bitstream.h
	color_attribute.h
	corto.h
	cstream.h
	decoder.h
	encoder.h
	index_attribute.h
	normal_attribute.h
	point.h
	tunstall.h
	vertex_attribute.h
	zpoint.h)

SET(SOURCES
	bitstream.cpp
	color_attribute.cpp
	cstream.cpp
	decoder.cpp
	encoder.cpp
	normal_attribute.cpp
	tunstall.cpp)

ADD_LIBRARY(corto STATIC ${SOURCES} ${HEADERS})

SET(DEFINITIONS_CORTO CORTO_STATIC) 

if(MSVC)
	SET(CFLAGS_CORTO /nologo /W3)
	LIST(APPEND DEFINITIONS_CORTO _CRT_SECURE_NO_DEPRECATE)
else()
	SET(CFLAGS_CORTO -O2 -W -Wall -c -std=c++11 -pedantic)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	LIST(APPEND CFLAGS_CORTO -fPIC)
elseif(APPLE)
	LIST(APPEND CFLAGS_CORTO -fvisibility=hidden)
	if(IOS)
		LIST(APPEND CFLAGS_CORTO -fembed-bitcode)
		set_xcode_property(corto IPHONEOS_DEPLOYMENT_TARGET "9.2")
	endif(IOS)
endif()

target_compile_definitions(corto PUBLIC ${DEFINITIONS_CORTO})
target_compile_options    (corto PUBLIC ${CFLAGS_CORTO})

INSTALL(TARGETS corto
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

INSTALL(FILES
	bitstream.h
	color_attribute.h
	corto.h
	cstream.h
	decoder.h
	encoder.h
	index_attribute.h
	normal_attribute.h
	point.h
	tunstall.h
	vertex_attribute.h
	zpoint.h
	DESTINATION include/corto)
