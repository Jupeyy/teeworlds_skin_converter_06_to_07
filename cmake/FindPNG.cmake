if(NOT(CMAKE_SYSTEM_NAME STREQUAL "Windows"))
set(CMAKE_MODULE_PATH ${ORIGINAL_CMAKE_MODULE_PATH})
find_package(PNG)
set(CMAKE_MODULE_PATH ${OWN_CMAKE_MODULE_PATH})
endif()

if(NOT PNG_FOUND)
    MESSAGE(STATUS "Searching for bundled PNG package in ${LIBRARY_SEARCH_ROOT_PATH}/${CMAKE_SYSTEM_NAME}")

    FIND_PATH(PNG_INCLUDE_DIRS
        NAMES png.h
        PATHS ${LIBRARY_SEARCH_ROOT_PATH}/${CMAKE_SYSTEM_NAME}/PNG/include
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
    )

    FIND_LIBRARY(PNG_LIBRARIES
        NAMES png libpng.lib libpng_static.lib libpng.a
        PATHS ${LIBRARY_SEARCH_ROOT_PATH}/${CMAKE_SYSTEM_NAME}/PNG/lib/${TARGET_CPU_ARCHITECTURE}
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
    )

	IF(PNG_INCLUDE_DIRS AND PNG_LIBRARIES)
		SET(PNG_FOUND TRUE)
	ELSE(PNG_INCLUDE_DIRS AND PNG_LIBRARIES)
		SET(PNG_FOUND FALSE)
	ENDIF(PNG_INCLUDE_DIRS AND PNG_LIBRARIES)
endif()

IF(PNG_FOUND)
    MESSAGE(STATUS "Found PNG includes at ${PNG_INCLUDE_DIRS}")
    MESSAGE(STATUS "Found PNG library at ${PNG_LIBRARIES}")
ELSE(PNG_FOUND)
    MESSAGE(FATAL_ERROR "Could NOT find PNG library")
ENDIF(PNG_FOUND)

MARK_AS_ADVANCED(
    PNG_INCLUDE_DIRS
)
