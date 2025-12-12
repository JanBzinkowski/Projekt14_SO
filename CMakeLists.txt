cmake_minimum_required(VERSION 4.0)
project(SO)
set(CMAKE_CXX_STANDARD 26)

add_executable(SO src/main.cpp
        include/Shared_memory.h
        include/Tables.h)
add_executable(klient src/klient.cpp
        include/zamowienie.h)
add_executable(generator_klientow src/generator_klientow.cpp)
add_executable(pracownik src/pracownik.cpp
        include/zamowienie.h
        include/Shared_memory.h
        include/Tables.h)
add_executable(kierownik src/kierownik.cpp)
