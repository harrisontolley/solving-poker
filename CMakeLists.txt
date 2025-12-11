cmake_minimum_required(VERSION 3.15)

# Project metadata
project(ComputerPoker
    VERSION 0.1.0
    LANGUAGES CXX
)

# global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# Library target for game logic
add_library(kuhn_lib
Kuhn/kuhngame.cpp
Leduc/leducgame.cpp
)

# Include directories for kuhn_lib
target_include_directories(kuhn_lib
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Kuhn
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Leduc
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(kuhn_lib PRIVATE
        -Wall -Wextra -pedantic
    )
endif()


# Executable target
add_executable(kuhn Kuhn/main.cpp)
# Link executable to game logic
target_link_libraries(kuhn PRIVATE kuhn_lib)


add_executable(leduc Leduc/main.cpp)
target_link_libraries(leduc PRIVATE kuhn_lib)