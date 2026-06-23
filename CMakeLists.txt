cmake_minimum_required(VERSION 3.15)
project(Stegozaur VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FLAGS -Wall -Wextra -Wpedantic -Werror)

set(SOURCES
    src/image.cpp
    src/audio.cpp
    src/lsb_algorithm.cpp
    src/stegozaur.cpp
    src/lsb_matching_algorithm.cpp
    src/pvd_algorithm.cpp
)

set(THIRD_PARTY_SOURCES
    src/stb_image_impl.cpp
    src/dr_wav_impl.cpp
)

add_library(third_party_lib STATIC ${THIRD_PARTY_SOURCES})

target_include_directories(third_party_lib PRIVATE third_party)

add_executable(${PROJECT_NAME} ${SOURCES} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    third_party
)

target_compile_options(${PROJECT_NAME} PRIVATE ${FLAGS})

target_link_libraries(${PROJECT_NAME} PRIVATE third_party_lib)
