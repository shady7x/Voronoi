cmake_minimum_required(VERSION 3.16)
project(Voronoi VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
include_directories(Voronoi ${SDL2_INCLUDE_DIRS})

add_executable(Voronoi Voronoi.cpp)
target_link_libraries(Voronoi ${SDL2_LIBRARIES})