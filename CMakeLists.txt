cmake_minimum_required(VERSION 3.25)

project(HailoPlayground)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g -DDBG")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -O3")

set(default_build_type "Debug")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)


find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
find_package(HailoRT REQUIRED)
find_package(OpenCV REQUIRED)

# Build the "classify" binary
add_executable(
    classify
    src/classify.cpp
    src/Hailo8Device.cpp
)

target_include_directories(
    classify PRIVATE 
    ${HailoRT_INCLUDE_DIRS} 
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(
    classify 
    HailoRT::libhailort 
    ${OpenCV_LIBS}
)
# end "classify"

# build "detect" binary
add_executable(
    detect
    src/detect.cpp
    src/Hailo8Device.cpp
    src/EmailNotifier.cpp
)

target_include_directories(
    detect PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${HailoRT_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(
    detect
    CURL::libcurl
    HailoRT::libhailort
    ${OpenCV_LIBS}
)
# end "detect"