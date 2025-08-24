project(EncodingConverter)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Add high DPI support definitions
add_definitions(-DQT_NO_DEPRECATED_WARNINGS=0)
if(WIN32)
    add_definitions(-DQT_WIN_HIGHDPI_SUPPORT)
endif()

find_package(Qt5 CONFIG COMPONENTS Widgets)

# Enable C++17 standard to support filesystem and high DPI features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp MainWindow.h MainWindow.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_link_libraries(${PROJECT_NAME} PRIVATE Iconv::Iconv uchardet::libuchardet Qt::Widgets)
