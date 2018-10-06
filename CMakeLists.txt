cmake_minimum_required (VERSION 2.8)
project (gouda)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -lPocoFoundation -lPocoNet -lPocoNetSsl")

set (GOUDA_VERSION_MAJOR 4)
set (GOUDA_VERSION_MINOR 0)
set (GOUDA_VERSION_PATCH 0)

include_directories (include)

file(GLOB SOURCES "src/*.cpp")

add_executable (Gouda ${SOURCES})
target_link_libraries (Gouda ${CPR_LIBRARIES})
