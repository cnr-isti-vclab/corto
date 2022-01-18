cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(RoomleCore VERSION 1.5.1 )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmakeModules")
include( Roomle )

option(ADD_DEBUG_INFO "compile with debug information" OFF)

#for gcc
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 14)

add_compile_definitions(
    TOKEN_USE_SHARED_STRING_ALLOWED
    ALLOW_CONST_STRUCT
    RAPIDJSON_HAS_STDSTRING
)

include_directories(
        lib/corto/include/corto
)

option( ROOMLE_TEST "Create Unit Test executables" ON )
option( ROOMLE_CBIND_STANDALONE "Build the CBinding for a standalone application (like Unity Editor)." OFF )
set( ROOMLE_UNITY_PROJECT_PATHS "" CACHE PATH "Path of the Roomle Unity 3D project. Libraries will be built into the correct subfolders, if set." )
option(BUILD_PYTHON "Build Python Library" OFF)


configure_file (
	cmakeModules/version.cpp.in
	"${PROJECT_BINARY_DIR}/cppCore/version.cpp"
)

# By default all dependencies are NOT built (i.e. BUILD_DEPS=OFF).
# IF building any wrapper THEN Force BUILD_DEPS=ON
# IF BUILD_DEPS=ON THEN Force all BUILD_*=ON
include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(BUILD_DEPS "Force re-build of all dependencies" OFF
  "NOT BUILD_PYTHON; NOT BUILD_JAVA; NOT BUILD_DOTNET" ON)

message(STATUS "Build all dependencies: ${BUILD_DEPS}")

set( LibType STATIC )
set( ROOMLE_TEST_SUPPORTED ON )

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set( DEBUG TRUE )
    add_compile_definitions( TESTING_AND_ASSERTS )
    add_compile_options(
        -Wall
        -Wextra
        -Wsign-conversion
        -Wno-unused-parameter
        -Wno-unknown-pragmas    # because of ParserATNSimulator
        -Wno-attributes         # because of ATNState
    )
    if(NOT EMSCRIPTEN)
        add_compile_options(
            -Werror
        )
    else()
        # disable additional warnings for emcc 2.0
        add_compile_options(
            -Wno-unused-command-line-argument
            -Wno-return-stack-address
            -Wno-unknown-pragmas
            -Wno-unused-but-set-variable
            -Qunused-arguments
        )
    endif()
    if(ADD_DEBUG_INFO)
        add_compile_options(
            -g
            -O0
        )
    endif()
endif()

if(IOS)
    set( ROOMLE_INTERFACE_C ON )
    set( ROOMLE_INTERFACE_IOS ON )
    set( ROOMLE_TEST_SUPPORTED OFF )

	set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "FH8K5HPPMW" CACHE STRING "Apple Development Team ID (for signing)" )
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES )
	set(CMAKE_XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
	set(CMAKE_XCODE_ATTRIBUTE_SDKROOT "iphoneos")
	set(CMAKE_XCODE_ATTRIBUTE_ARCHS "$(ARCHS_STANDARD)")
	set(CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "s")
	set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "8.0")
endif()

if(ANDROID)
    set( ROOMLE_INTERFACE_C ON )
    set( ROOMLE_INTERFACE_JNI ON )
endif()

if(EMSCRIPTEN)
    set( ROOMLE_INTERFACE_EMSCRIPTEN ON )
    set( ROOMLE_TEST_SUPPORTED OFF )

    option( ROOMLE_THREEJS_BOUNDARY "Generate Embind for high speed configurator" OFF)
    set( ROOMLE_EMSCRIPTEN_MEMORY_SIZE "167772160" CACHE STRING "WebGL Memory Size (in bytes)" )
    option( ROOMLE_ALLOW_MEMORY_GROWTH "Allow memory growth in WebGL" ON )
endif()

if(ROOMLE_CBIND_STANDALONE)
    set( ROOMLE_INTERFACE_C ON )
    set( ROOMLE_INTERFACE_IOS OFF )
    set( ROOMLE_INTERFACE_JNI OFF )
    set( ROOMLE_TEST_SUPPORTED OFF )

    if( APPLE AND NOT IOS )
        # Unity Editor (on macOS) requires a dynamic library.
        set( LibType MODULE )
    endif()
endif()


set( commonSrc
	cppCore/null.cpp
	$<TARGET_OBJECTS:objLogger>
    $<TARGET_OBJECTS:objModel>
    $<TARGET_OBJECTS:objUtil>
    $<TARGET_OBJECTS:objComponentKernelCommon>
	$<TARGET_OBJECTS:objRoomleGrammar>
    )

set( roomleCoreSrc
	"${PROJECT_BINARY_DIR}/cppCore/version.cpp"
	$<TARGET_OBJECTS:objPlannerComponentKernel>
	$<TARGET_OBJECTS:objPlannerModel>
	$<TARGET_OBJECTS:objPlannerUtil>
	$<TARGET_OBJECTS:objPlannerInteraction>
        $<TARGET_OBJECTS:objRoomleGrammar>
	)

set( configuratorKernelStandaloneSrc
	$<TARGET_OBJECTS:objComponentKernelStandalone>
        $<TARGET_OBJECTS:objRoomleGrammar>
	)

if(ROOMLE_INTERFACE_IOS)
	list( APPEND roomleCoreSrc
        $<TARGET_OBJECTS:objInterfaceConfiguratorIOS>
        )
elseif(ROOMLE_INTERFACE_JNI)
	list( APPEND roomleCoreSrc
		$<TARGET_OBJECTS:objInterfaceJNI>
		)
endif()

if(EMSCRIPTEN)

    set( EMSCRIPTEN_OPTIONS
        "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
        "SHELL:-s FILESYSTEM=0"
        "SHELL:-s MODULARIZE=1"
        "--bind"
        )

    add_compile_definitions( USE_MAKE_SHARED_BYPASS )
    add_compile_options( ${EMSCRIPTEN_OPTIONS} )
    add_link_options( ${EMSCRIPTEN_OPTIONS} )

    if (DEBUG)
        set( EMSCRIPTEN_OPTIONS_DEBUG
            -g
            "SHELL:-s ALLOW_MEMORY_GROWTH=1"
            "SHELL:-s DEMANGLE_SUPPORT=1"
            "SHELL:-s ASSERTIONS=2"
        )
        add_compile_options( ${EMSCRIPTEN_OPTIONS_DEBUG} )
        add_link_options( ${EMSCRIPTEN_OPTIONS_DEBUG} )

    else(DEBUG)
        # Optimizations
        # see <https://emscripten.org/docs/optimizing/Optimizing-Code.html>
        set( EMSCRIPTEN_OPTIONS_RELEASE
            -Oz # Agressively reduce code size at the expense of performance
            -flto # link time optimization
            "SHELL:-s ENVIRONMENT=web" # will emit code that only runs on the Web, and does not include support code for Node.js and other environments.
            "SHELL:--closure 1" # Use the closure compiler; reduce the size of the support JavaScript code
            "SHELL:--llvm-opts 3"
        )
        add_compile_options( ${EMSCRIPTEN_OPTIONS_RELEASE} )
        add_link_options( ${EMSCRIPTEN_OPTIONS_RELEASE} )

        if(ROOMLE_EMSCRIPTEN_MEMORY_SIZE)
            if (ROOMLE_EMSCRIPTEN_MEMORY_SIZE MATCHES "^[0-9]+$")
                add_link_options(
                    "SHELL:-s TOTAL_MEMORY=${ROOMLE_EMSCRIPTEN_MEMORY_SIZE}"
                )
            else()
                message( WARNING "ROOMLE_EMSCRIPTEN_MEMORY_SIZE must be a valid number!" )
            endif()
        else()
            message( WARNING "ROOMLE_EMSCRIPTEN_MEMORY_SIZE not set!" )
        endif()

        if(ROOMLE_ALLOW_MEMORY_GROWTH)
            add_compile_options("SHELL:-s ALLOW_MEMORY_GROWTH=1")
            add_link_options("SHELL:-s ALLOW_MEMORY_GROWTH=1")
        endif()
    endif()

    list( APPEND roomleCoreSrc
        $<TARGET_OBJECTS:objInterfacePlannerJs>
        )

    list( APPEND configuratorKernelStandaloneSrc
        $<TARGET_OBJECTS:objInterfaceJs>
        )

    add_executable( ConfiguratorKernel
        ${commonSrc}
        ${configuratorKernelStandaloneSrc}
        )

    add_executable( RoomleCore
        ${commonSrc}
        ${roomleCoreSrc}
        )

    add_dependencies(RoomleCore antlr4_static)    
    target_compile_options(RoomleCore PRIVATE "SHELL:-s WASM=1")
    target_link_options(RoomleCore PRIVATE "SHELL:-s WASM=1" "SHELL:-s EXPORT_NAME=\"'RoomleCore'\"")

    add_dependencies(ConfiguratorKernel antlr4_static)
    target_compile_definitions( ConfiguratorKernel PUBLIC CONFIGURATOR_STANDALONE )
    target_compile_options( ConfiguratorKernel PRIVATE "SHELL:-s WASM=1" )
    target_link_options( ConfiguratorKernel PRIVATE "SHELL:-s WASM=1" "SHELL:-s EXPORT_NAME=\"'ConfiguratorKernel'\"" )

    # BEGIN ASM.JS (obsolete)

    # Additional build variant for asm.js
    add_executable( ConfiguratorKernelJs
        ${commonSrc}
        ${configuratorKernelStandaloneSrc}
        )
    add_dependencies(ConfiguratorKernelJs antlr4_static)
    target_link_libraries(ConfiguratorKernelJs PUBLIC corto antlr4_static)

    # Additional build variant for asm.js
    add_executable( RoomleCoreJs
        ${commonSrc}
        ${roomleCoreSrc}
        )
    add_dependencies(RoomleCoreJs antlr4_static )
    target_link_libraries(RoomleCoreJs PUBLIC corto antlr4_static)

    set(EMSCRIPTEN_ASMJS_OPTIONS
        "SHELL:-s WASM=0"
        "SHELL:--memory-init-file 1"
        # Uncomment to create HTML test template
        # "SHELL:-o Test.html"
    )
    target_compile_options( RoomleCoreJs PRIVATE ${EMSCRIPTEN_ASMJS_OPTIONS} )
    target_link_options( RoomleCoreJs PRIVATE ${EMSCRIPTEN_ASMJS_OPTIONS} "SHELL:-s EXPORT_NAME=\"'RoomleCore'\"" )

    target_compile_definitions( ConfiguratorKernelJs PUBLIC CONFIGURATOR_STANDALONE )
    target_compile_options( ConfiguratorKernelJs PRIVATE ${EMSCRIPTEN_ASMJS_OPTIONS} )
    target_link_options( ConfiguratorKernelJs PRIVATE ${EMSCRIPTEN_ASMJS_OPTIONS} "SHELL:-s EXPORT_NAME=\"'ConfiguratorKernel'\"" )

    # END ASM.JS (obsolete)

else()

	add_library( ConfiguratorKernel
		${LibType}
		${commonSrc}
		${configuratorKernelStandaloneSrc}
		)
	add_library( RoomleCore
		${LibType}
		${commonSrc}
		${roomleCoreSrc}
		)

endif()

# These are some global include directory variables.
# This is a workaround to avoid (even worse) global include_directory commands.
# Ideally every sub-library/module has its own header path and not many outside references.
# Some internal/private, some public (exposed to the using lib/exe)
set( rootDir $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> )
set( cppCoreDir $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/cppCore )
set( loggerDir $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/cppCore/logger )
set( configuratorDir $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/cppCore/configurator/inc )
set(libDir $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/lib)
set(antlrRuntimeDir ${libDir}/antlr/runtime/src)

add_subdirectory( lib )
add_subdirectory( cppCore )

if( ROOMLE_INTERFACE_C )
    add_subdirectory( interfaces/CBinding )
endif()
if( ROOMLE_INTERFACE_IOS )
    add_subdirectory( interfaces/iOS )
endif()
if( ROOMLE_INTERFACE_JNI )
    add_subdirectory( interfaces/jni )
endif()
if( EMSCRIPTEN )
	add_subdirectory( interfaces/emscripten )
endif()

add_dependencies(RoomleCore antlr4_static )
add_dependencies(ConfiguratorKernel antlr4_static )

if(EMSCRIPTEN)
    target_link_libraries(RoomleCore PUBLIC antlr4_static)
    target_link_libraries(ConfiguratorKernel PUBLIC antlr4_static)
else()
    target_link_libraries(RoomleCore PUBLIC corto antlr4_static)
    target_link_libraries(ConfiguratorKernel PUBLIC corto antlr4_static)
endif()

if(ROOMLE_TEST)
	if( NOT ROOMLE_TEST_SUPPORTED )
	message( WARNING "Tests are not supported in this platform/feature combination." )
	set( ROOMLE_TEST OFF )
	else()
	add_subdirectory( test )
	endif()
endif()

if(ROOMLE_INTERFACE_C)
	target_sources( ConfiguratorKernel
		PRIVATE
		$<TARGET_OBJECTS:objInterfaceCBinding>
		)

	target_sources( RoomleCore
		PRIVATE
		$<TARGET_OBJECTS:objInterfacePlannerCBinding>
		)
endif()

target_include_directories( RoomleCore
	PUBLIC
	${cppCoreDir}
	${configuratorDir}
	${libDir}
        ${loggerDir}
        ${antlrRuntimeDir}
	)

target_compile_definitions( ConfiguratorKernel PRIVATE CONFIGURATOR_STANDALONE )

if(ROOMLE_CBIND_STANDALONE)
    target_compile_definitions( RoomleCore PRIVATE ROOMLECORE_AUTONOMIC )
    
    if( APPLE AND NOT IOS)
        # Unity Editor need the library in .bundle format:
        set_target_properties(ConfiguratorKernel PROPERTIES BUNDLE true)
        set_target_properties(RoomleCore PROPERTIES BUNDLE true)
    endif()
endif()

if(ROOMLE_CBIND_STANDALONE AND ROOMLE_UNITY_PROJECT_PATHS)
	foreach(ROOMLE_UNITY_PROJECT_PATH ${ROOMLE_UNITY_PROJECT_PATHS})
		set( DEST_PLUGIN_PATH "${ROOMLE_UNITY_PROJECT_PATH}/Assets/Plugins")

		if(ANDROID)
			set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/Android/libs/${ANDROID_ABI})
		elseif(IOS)
			set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/iOS)
		else()
			if(bitness EQUAL 32)
				set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/x86)
			else()
				set(UNITY_PLUGIN_DIR ${DEST_PLUGIN_PATH}/x86_64)
			endif()
		endif()

		message( "Will install library to ${UNITY_PLUGIN_DIR}" )

		install(TARGETS RoomleCore DESTINATION ${UNITY_PLUGIN_DIR})
		install(TARGETS ConfiguratorKernel DESTINATION ${UNITY_PLUGIN_DIR})
	endforeach()
endif()


if(BUILD_PYTHON)
	# interface cpp is only used for python atm
	add_subdirectory(interfaces/cpp)
	include(python)
endif()
