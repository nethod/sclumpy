cmake_minimum_required(VERSION 3.20)
project(meshtastic_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(meshtastic_monitor
    src/main.cpp
)

target_compile_definitions(meshtastic_monitor PRIVATE UNICODE _UNICODE)
