cmake_minimum_required(VERSION 3.16)
project(geometricimagesearch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory is build/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# Find all *.cpp files in source/ 
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# Executable lies in program/(search.cpp)
add_executable(search
    program/search.cpp
    ${SOURCES}
)

# Compiler flags
target_compile_options(search PRIVATE -Werror -Wextra -Wpedantic)

# Include directory is include/
target_include_directories(search PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link with opencv library
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(search ${OpenCV_LIBS})