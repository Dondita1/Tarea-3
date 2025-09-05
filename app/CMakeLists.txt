cmake_minimum_required(VERSION 3.10)

project(cursos_console_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cursos
    src/main.cpp
)

if (MSVC)
    target_compile_options(cursos PRIVATE /W4 /permissive-)
else()
    target_compile_options(cursos PRIVATE -Wall -Wextra -Wpedantic)
endif()

