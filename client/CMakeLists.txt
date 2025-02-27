cmake_minimum_required(VERSION 3.15)
project(Client)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS_RELEASE "-O2 -Wall -Wextra" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Wextra" CACHE STRING "" FORCE)

file(GLOB CLIENT
        "${Client_SOURCE_DIR}/src/*.cpp"
        "${Client_SOURCE_DIR}/src/*.h"
)

add_executable(Client
    ${CLIENT})

target_link_libraries(Client
    LINK_PRIVATE
    -pthread
)
