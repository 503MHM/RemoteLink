cmake_minimum_required(VERSION 3.19)

project(RemoteLink VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Widgets
    Network
)

qt_standard_project_setup()

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)