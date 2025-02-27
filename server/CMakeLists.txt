cmake_minimum_required(VERSION 3.15)
project(Server)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS_RELEASE "-O2 -Wall -Wextra" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Wextra" CACHE STRING "" FORCE)

file(GLOB SERVER
        "${Server_SOURCE_DIR}/src/*.cpp"
        "${Server_SOURCE_DIR}/src/*.h"
)

add_executable(Server
    ${SERVER})

target_link_libraries(Server
    LINK_PRIVATE
    -pthread
)

