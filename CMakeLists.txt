#Minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.0)

# Name of the project
project(CVDConvert)

# Add all the source files needed to build the executable
add_executable(CVDConvert main.cpp)

# -DCMAKE_BUILD_TYPE=DEBUG to build 
# -DCMAKE_BUILD_TYPE=RELEASE to build release