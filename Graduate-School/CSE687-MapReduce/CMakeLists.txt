cmake_minimum_required(VERSION 3.10)

project(MapReduce)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files
add_executable(MapReduce
    main.cpp
    mapper.cpp
    reducer.cpp
    fileHandler.cpp
    utils.cpp
)

# Include directories
target_include_directories(MapReduce PRIVATE ${CMAKE_SOURCE_DIR})