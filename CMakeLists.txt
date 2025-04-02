cmake_minimum_required(VERSION 3.16)
project(JaDe-AudioProcessor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Charts REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Charts REQUIRED)
find_package(SndFile REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES "src/*.cpp")

add_executable(JaDe-AudioProcessor ${SOURCES})

target_link_libraries(JaDe-AudioProcessor
    Qt6::Widgets
    Qt6::Charts
    SndFile::sndfile
)