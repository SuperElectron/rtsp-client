################################################
### Build Instructions
# mkdir build && cd build
# cmake ..
# cmake --build . -j
################################################

################################################
# Main CMake settings for the project
################################################
cmake_minimum_required(VERSION 3.15.2)
enable_language(C CXX)

# set RAD_LIB descriptors
set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "media server for rtsp stream ingestion")

# set the C++ standard
set(CMAKE_BUILD_TYPE Debug) # Release or Debug
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

################################################
# set directories
################################################

set(CMAKE_CONFIGURATION_TYPES UNIX)
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(PROJECT_MODULES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/modules)

message("CMAKE_BINARY_DIR: " ${CMAKE_BINARY_DIR})
message("CMAKE_RUNTIME_OUTPUT_DIRECTORY: " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message("PROJECT_MODULES_DIRECTORY: " ${PROJECT_MODULES_DIRECTORY})

################################################
# Configure project executable and library
################################################

project(rtsp_client DESCRIPTION ${PROJECT_DESCRIPTION} VERSION ${PROJECT_VERSION})
add_library(${PROJECT_NAME}_LIB STATIC)
set_target_properties(${PROJECT_NAME}_LIB PROPERTIES POSITION_INDEPENDENT_CODE ON)

## Get source files
set(SRC_FILE ./main.cpp)
# get the source files
file(GLOB APPLICATION_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
        )

file(GLOB APPLICATION_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        )

## include this module's directory so other modules can find it
#target_include_directories(${PROJECT_NAME}_LIB PUBLIC .)

# create the library
target_sources(${PROJECT_NAME}_LIB PUBLIC
        ${APPLICATION_HEADERS}
        ${APPLICATION_SRC}
        )
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
add_executable(${PROJECT_NAME} ${SRC_FILE})

# include libs
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER gstreamer-1.0>=1.16 REQUIRED)
pkg_check_modules(GSTREAMERAPP gstreamer-app-1.0 REQUIRED)
pkg_check_modules(GSTVIDEO gstreamer-video-1.0>=1.16 REQUIRED)
pkg_check_modules(GLIB2 glib-2.0 REQUIRED)
pkg_check_modules(JSON json-glib-1.0 REQUIRED)

target_include_directories(${PROJECT_NAME}_LIB PUBLIC
        ${GLIB2_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
        ${GSTREAMERAPP_INCLUDE_DIRS}
        ${GSTVIDEO_INCLUDE_DIRS}
        ${JSON_INCLUDE_DIRS}
        ${THREAD_INCLUDE_DIRS}
        )

target_link_libraries(${PROJECT_NAME}_LIB PUBLIC
        ${GLIB2_LIBRARIES}
        ${GSTREAMER_LDFLAGS}
        ${GSTREAMERAPP_LDFLAGS}
        ${GSTVIDEO_LDFLAGS}
        ${JSON_LDFLAGS}
        ${THREAD_LDFLAGS}
        ${CMAKE_THREAD_LIBS_INIT}
        )

################################################
## Build Application
################################################

set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD 20)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_features(${PROJECT_NAME}_LIB PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_LIB)

################################################
# Set additional library properties
################################################

set_target_properties(${PROJECT_NAME}_LIB PROPERTIES
        VERSION ${PROJECT_VERSION}
        LINK_FLAGS "-Wl,--whole-archive"
        )
target_include_directories(${PROJECT_NAME}_LIB PUBLIC ${PROJECT_MODULES_DIRECTORY})
include(GNUInstallDirs)
message(CMAKE_INSTALL_LIBDIR: ${CMAKE_INSTALL_LIBDIR})
message(CMAKE_INSTALL_INCLUDEDIR: ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS ${PROJECT_NAME}_LIB
        LIBRARY DESTINATION ${CMAKE_BINARY_DIR}
        DESTINATION ${CMAKE_BINARY_DIR}
        )
