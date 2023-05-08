cmake_minimum_required(VERSION 3.16)
project(voronoi VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall")

if (WIN32) 
    execute_process(COMMAND cmd "/c" "glslc ../shaders/shader.vert -o vert.spv")
    execute_process(COMMAND cmd "/c" "glslc ../shaders/shader.frag -o frag.spv")
else()
    execute_process(COMMAND bash "-c" "glslc ../shaders/shader.vert -o vert.spv")
    execute_process(COMMAND bash "-c" "glslc ../shaders/shader.frag -o frag.spv")
endif()

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
add_executable(voronoi voronoi.cpp)

file(GLOB SOURCES_SRC "voronoi.cpp" "vulkan_engine.cpp")
target_sources(voronoi PUBLIC ${SOURCES_SRC})
target_include_directories(voronoi PUBLIC ${SDL2_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(voronoi ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES})