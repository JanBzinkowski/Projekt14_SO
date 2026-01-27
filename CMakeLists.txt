cmake_minimum_required(VERSION 3.20)
project(mainprog)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

add_executable(klient src/klient.cpp src/Wrappers.cpp
        src/Tables.cpp)
add_executable(generator_klientow src/generator_klientow.cpp src/Wrappers.cpp src/Tables.cpp)
add_executable(pracownik src/pracownik.cpp src/Wrappers.cpp
        src/Tables.cpp)
add_executable(kierownik src/kierownik.cpp src/Wrappers.cpp src/Tables.cpp)
add_executable(mainprog src/mainprog.cpp
        src/Tables.cpp src/Wrappers.cpp)
add_executable(logger src/Logger.cpp src/Wrappers.cpp src/Tables.cpp)
add_executable(kasjer src/kasjer.cpp src/Wrappers.cpp src/Tables.cpp)