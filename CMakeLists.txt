cmake_minimum_required(VERSION 3.15)

project(ComputerPoker
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Supports either:
#   ./SKPokerEval
#   ./src/SKPokerEval

set(SKP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SKPokerEval")
if(NOT EXISTS "${SKP_DIR}/src/SevenEval.h")
    set(SKP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/SKPokerEval")
endif()

if(NOT EXISTS "${SKP_DIR}/src/SevenEval.h")
    message(FATAL_ERROR
        "Could not find SKPokerEval. Expected SevenEval.h at:\n"
        "  ${CMAKE_CURRENT_SOURCE_DIR}/SKPokerEval/src/SevenEval.h\n"
        "or\n"
        "  ${CMAKE_CURRENT_SOURCE_DIR}/src/SKPokerEval/src/SevenEval.h\n"
        "Fix the clone location or update SKP_DIR in CMakeLists.txt."
    )
endif()

# Build SKPokerEval as a static library by compiling its src/*.cpp
file(GLOB SKPOKEREVAL_SOURCES CONFIGURE_DEPENDS
    "${SKP_DIR}/src/*.cpp"
)

add_library(skpokereval STATIC ${SKPOKEREVAL_SOURCES})

target_include_directories(skpokereval
    SYSTEM PUBLIC
        "${SKP_DIR}/src"
)

target_compile_features(skpokereval PUBLIC cxx_std_20)

add_library(poker_lib
    Kuhn/kuhngame.cpp
    Leduc/leducgame.cpp
    HeadsUp/headsupgame.cpp
)

target_include_directories(poker_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Kuhn
        ${CMAKE_CURRENT_SOURCE_DIR}/Leduc
        ${CMAKE_CURRENT_SOURCE_DIR}/HeadsUp
        ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# This is what makes SevenEval.h visible when compiling poker_lib and links the evaluator objects in at link time.
target_link_libraries(poker_lib PUBLIC skpokereval)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(poker_lib PRIVATE -Wall -Wextra -pedantic)
endif()

# --- Executables ---
add_executable(kuhn Kuhn/main.cpp)
target_link_libraries(kuhn PRIVATE poker_lib)

add_executable(leduc Leduc/main.cpp)
target_link_libraries(leduc PRIVATE poker_lib)

add_executable(headsup HeadsUp/main.cpp)
target_link_libraries(headsup PRIVATE poker_lib)
