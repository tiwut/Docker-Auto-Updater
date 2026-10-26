cmake_minimum_required(VERSION 3.16)
project(DockerUpdater VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ADWAITA REQUIRED libadwaita-1)

add_executable(DockerUpdater
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_include_directories(DockerUpdater PRIVATE ${ADWAITA_INCLUDE_DIRS})
target_compile_options(DockerUpdater PRIVATE ${ADWAITA_CFLAGS_OTHER})
target_link_libraries(DockerUpdater PRIVATE ${ADWAITA_LIBRARIES} pthread)
