cmake_minimum_required(VERSION 3.4)

project(Converter)

set(Converter_MAJOR_VERSION 0)
set(Converter_MINOR_VERSION 1)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(TARGET_BITS "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(TARGET_BITS "32")
endif()

if(TARGET_BITS STREQUAL "32")
	set(TARGET_CPU_ARCHITECTURE "x86")
else()
	set(TARGET_CPU_ARCHITECTURE "x86_64")
endif()

set(CMAKE_CXX_STANDARD 14)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()
message("Build mode: ${CMAKE_BUILD_TYPE}")

#######################################################
# add dependencies
#######################################################
if(NOT LIBRARY_SEARCH_ROOT_PATH)
	set(LIBRARY_SEARCH_ROOT_PATH "${CMAKE_SOURCE_DIR}/libs")
endif()

set(OWN_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${OWN_CMAKE_MODULE_PATH})

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

include_directories("${PNG_INCLUDE_DIRS};${ZLIB_INCLUDE_DIRS};${CMAKE_SOURCE_DIR}")

set(COMPILE_SOURCE)
list(APPEND COMPILE_SOURCE "Main.cpp"
"Converter.cpp" "Converter.h"
)

add_executable(Converter ${COMPILE_SOURCE})
if(NOT(CMAKE_SYSTEM_NAME STREQUAL "Windows"))
target_link_libraries(Converter ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
else()
target_link_libraries(Converter ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} -static -static-libgcc -static-libstdc++)
endif()
