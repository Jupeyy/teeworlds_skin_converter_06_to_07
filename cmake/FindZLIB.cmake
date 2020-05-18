if(NOT(CMAKE_SYSTEM_NAME STREQUAL "Windows"))
set(CMAKE_MODULE_PATH ${ORIGINAL_CMAKE_MODULE_PATH})
find_package(ZLIB)
set(CMAKE_MODULE_PATH ${OWN_CMAKE_MODULE_PATH})
endif()

if(NOT ZLIB_FOUND)
    MESSAGE(STATUS "Searching for bundled ZLIB package in ${LIBRARY_SEARCH_ROOT_PATH}/${CMAKE_SYSTEM_NAME}")

    FIND_PATH(ZLIB_INCLUDE_DIRS
        NAMES zlib.h
        PATHS ${LIBRARY_SEARCH_ROOT_PATH}/${CMAKE_SYSTEM_NAME}/zlib/include
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
    )	

    FIND_PATH(ZLIB_INCLUDE_DIRS_CONFIG
        NAMES zconf.h
        PATHS ${LIBRARY_SEARCH_ROOT_PATH}/${CMAKE_SYSTEM_NAME}/zlib/include/${TARGET_CPU_ARCHITECTURE}/include
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
    )

    FIND_LIBRARY(ZLIB_LIBRARIES
        NAMES zlib libzlib.lib zlibstatic.lib libz libz.a libzlibstatic.a
        PATHS ${LIBRARY_SEARCH_ROOT_PATH}/${CMAKE_SYSTEM_NAME}/zlib/lib/${TARGET_CPU_ARCHITECTURE}
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
    )
	
	IF(ZLIB_INCLUDE_DIRS AND ZLIB_INCLUDE_DIRS_CONFIG AND ZLIB_LIBRARIES)
		SET(ZLIB_FOUND TRUE)
	ELSE(ZLIB_INCLUDE_DIRS AND ZLIB_INCLUDE_DIRS_CONFIG AND ZLIB_LIBRARIES)
		SET(ZLIB_FOUND FALSE)
	ENDIF(ZLIB_INCLUDE_DIRS AND ZLIB_INCLUDE_DIRS_CONFIG AND ZLIB_LIBRARIES)
endif()

IF(ZLIB_FOUND)
    MESSAGE(STATUS "Found ZLIB includes at ${ZLIB_INCLUDE_DIRS}")
    MESSAGE(STATUS "Found ZLIB library at ${ZLIB_LIBRARIES}")
ELSE(ZLIB_FOUND)
    MESSAGE(FATAL_ERROR "Could NOT find ZLIB library")
ENDIF(ZLIB_FOUND)

MARK_AS_ADVANCED(
    ZLIB_INCLUDE_DIRS
)
