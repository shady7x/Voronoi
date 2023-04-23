cmake_minimum_required(VERSION 3.16)
project(voronoi VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(VK_LAYER_PATH "C:\\msys64\\ucrt64\\bin")
set(VK_SDK_PATH "C:\\msys64\\ucrt64\\bin")
set(VULKAN_SDK "C:\\msys64\\ucrt64\\bin")

set(CMAKE_CXX_FLAGS "-Wall")

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
add_executable(voronoi voronoi.cpp)

file(GLOB SOURCES_SRC "voronoi.cpp" "vulkan_engine.cpp")
target_sources(voronoi PUBLIC ${SOURCES_SRC})
target_include_directories(voronoi PUBLIC ${SDL2_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(voronoi ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES})