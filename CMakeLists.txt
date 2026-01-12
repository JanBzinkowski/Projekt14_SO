cmake_minimum_required(VERSION 3.20)
project(SO)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

add_executable(klient src/klient.cpp src/wrappers.cpp
        src/Tables.cpp)
add_executable(generator_klientow src/generator_klientow.cpp src/wrappers.cpp src/Tables.cpp)
add_executable(pracownik src/pracownik.cpp src/wrappers.cpp
        src/Tables.cpp)
add_executable(kierownik src/kierownik.cpp src/wrappers.cpp src/Tables.cpp)
add_executable(SO src/main.cpp
        src/Tables.cpp src/wrappers.cpp)
add_executable(logger src/Logger.cpp src/wrappers.cpp src/Tables.cpp)
add_executable(kasjer src/kasjer.cpp src/wrappers.cpp src/Tables.cpp)
