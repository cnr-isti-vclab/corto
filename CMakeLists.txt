cmake_minimum_required(VERSION 3.1)
project(corto)

option(BUILD_CORTO_CODEC_UNITY "Build the unity codec shared library of corto" ON)
option(BUILD_CORTO_EXE "Build the command line binary of corto" ON)
option(BUILD_CORTO_BENCHMARK	"Build the benchmark program of corto" ON)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

SET(CORTO_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
SET(CORTO_HEADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/corto)

SET(LIB_HEADERS
	${CORTO_HEADER_PATH}/bitstream.h
	${CORTO_HEADER_PATH}/color_attribute.h
	${CORTO_HEADER_PATH}/corto.h
	${CORTO_HEADER_PATH}/cstream.h
	${CORTO_HEADER_PATH}/decoder.h
	${CORTO_HEADER_PATH}/encoder.h
	${CORTO_HEADER_PATH}/index_attribute.h
	${CORTO_HEADER_PATH}/normal_attribute.h
	${CORTO_HEADER_PATH}/point.h
	${CORTO_HEADER_PATH}/tunstall.h
	${CORTO_HEADER_PATH}/vertex_attribute.h
	${CORTO_HEADER_PATH}/zpoint.h
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

SET(BENCHMARK_SOURCE
	${CORTO_SOURCE_PATH}/benchmark.cpp
	${CORTO_SOURCE_PATH}/meshloader.cpp
	${CORTO_SOURCE_PATH}/tinyply.cpp
)

SET(BENCHMARK_HEADERS
	${CORTO_SOURCE_PATH}/meshloader.h
	${CORTO_SOURCE_PATH}/tinyply.h
	${CORTO_SOURCE_PATH}/objload.h)

SET(CORTO_DEFINITIONS "")

if(MSVC)
	add_compile_options(/nologo /W3 -D_CRT_SECURE_NO_DEPRECATE)
else()
	add_compile_options(-Wall -pedantic)
endif()

ADD_LIBRARY(corto STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(corto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(corto PRIVATE ${CORTO_HEADER_PATH})
set_target_properties     (corto PROPERTIES DEBUG_POSTFIX "d")

INSTALL(TARGETS corto
	RUNTIME DESTINATION bin
	BUNDLE DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

INSTALL(FILES
	${CORTO_HEADER_PATH}/bitstream.h
	${CORTO_HEADER_PATH}/color_attribute.h
	${CORTO_HEADER_PATH}/corto.h
	${CORTO_HEADER_PATH}/cstream.h
	${CORTO_HEADER_PATH}/decoder.h
	${CORTO_HEADER_PATH}/encoder.h
	${CORTO_HEADER_PATH}/index_attribute.h
	${CORTO_HEADER_PATH}/normal_attribute.h
	${CORTO_HEADER_PATH}/point.h
	${CORTO_HEADER_PATH}/tunstall.h
	${CORTO_HEADER_PATH}/vertex_attribute.h
	${CORTO_HEADER_PATH}/zpoint.h
	DESTINATION include/corto)

if (BUILD_CORTO_CODEC_UNITY)
	ADD_LIBRARY(cortocodec_unity SHARED ${LIB_SOURCES} ${LIB_HEADERS})
	target_include_directories(cortocodec_unity PUBLIC ${CORTO_HEADER_PATH})
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
		# G++
		target_compile_options(cortocodec_unity PRIVATE -Wall -Wextra)
	elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
		# MSVC
		target_compile_options(cortocodec_unity PRIVATE /EHsc /MTd /W2 /c)
		# Set the DLLEXPORT variable to export symbols
		target_compile_definitions(cortocodec_unity PRIVATE WIN_EXPORT)
	endif()
endif()

if (BUILD_CORTO_EXE)
	ADD_EXECUTABLE(cortoexe ${EXE_SOURCE} ${EXE_HEADERS})
	target_include_directories(cortoexe PUBLIC ${CORTO_HEADER_PATH})
	target_link_libraries(cortoexe PUBLIC corto)
	set_target_properties(cortoexe PROPERTIES OUTPUT_NAME "corto")

	INSTALL(TARGETS cortoexe
		RUNTIME DESTINATION bin
		BUNDLE DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
endif()

if (BUILD_CORTO_BENCHMARK)
	ADD_EXECUTABLE(cortobenchmark ${BENCHMARK_SOURCE} ${BENCHMARK_HEADERS})
	add_subdirectory(deps/lz4/build/cmake)

	target_include_directories(cortobenchmark 
		PUBLIC ${CORTO_HEADER_PATH}
	)
	target_include_directories(corto
		PUBLIC deps/lz4/lib
	)

	target_link_libraries(cortobenchmark 
		PUBLIC corto
	)
	target_link_libraries(corto 
		PUBLIC lz4_static
	)
	
	target_compile_definitions(corto PUBLIC ENABLE_LZ4)

	set_target_properties(cortobenchmark PROPERTIES OUTPUT_NAME "corto_benchmark")

	INSTALL(TARGETS cortobenchmark
		RUNTIME DESTINATION bin
		BUNDLE DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
endif()