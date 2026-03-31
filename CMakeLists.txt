cmake_minimum_required(VERSION 3.16)
project(OS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(OS
    main.cpp
    backend/FAT32.cpp
    backend/Scheduler.cpp
    frontend/GUI.cpp
)

target_link_libraries(OS PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

target_include_directories(OS PRIVATE backend frontend)