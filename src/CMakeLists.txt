#Corto executable

SET(CORTOEXE_CFLAGS ${CORTO_CFLAGS})

set(EXE_SOURCE
	${CORTO_SOURCE_PATH}/main.cpp
	${CORTO_SOURCE_PATH}/meshloader.cpp
	${CORTO_SOURCE_PATH}/tinyply.cpp)

set(EXE_HEADERS
	${CORTO_SOURCE_PATH}/meshloader.h
	${CORTO_SOURCE_PATH}/tinyply.h
	${CORTO_SOURCE_PATH}/objload.h)

add_executable(cortoexe ${EXE_SOURCE} ${EXE_HEADERS})

target_include_directories(cortoexe PUBLIC ${CORTO_SOURCE_PATH})
target_link_libraries(cortoexe PUBLIC cortolib)
target_compile_options    (cortoexe PUBLIC ${CORTOEXE_CFLAGS})
set_target_properties     (cortoexe PROPERTIES OUTPUT_NAME "corto")
 
install(TARGETS cortoexe
  EXPORT CortoTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)
