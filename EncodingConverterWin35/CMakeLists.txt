cmake_minimum_required(VERSION 3.20)
project(EncodingConverterWin35)

set(CMAKE_CXX_STANDARD 17)

# Source files
set(SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    resources.rc
)

# Add vaca library
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Vaca as static library" FORCE)
add_subdirectory(vaca)

add_executable(EncodingConverterWin35 WIN32 ${SOURCES})

# Link libraries
target_link_libraries(EncodingConverterWin35 PRIVATE
    vaca
    comctl32
    uxtheme
    shell32
    ole32
    dwmapi
    Iconv::Iconv
    uchardet::libuchardet
)

# Include directories
target_include_directories(EncodingConverterWin35 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vaca
    ../common
)

target_compile_definitions(EncodingConverterWin35 PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    _WINDOWS
    VACA_STATIC
)

# Set subsystem to Windows and character set
set_target_properties(EncodingConverterWin35 PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_GLOBAL_CharacterSet "Unicode"
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /MANIFEST:NO"
    # Disable automatic manifest generation to avoid conflicts with our custom manifest
    VS_GLOBAL_EnableManagedIncrementalBuild "NO"
    VS_GLOBAL_GenerateManifest "FALSE"
)

# Set vaca library flags
set_target_properties(vaca PROPERTIES
    COMPILE_FLAGS "-DVACA_STATIC"
)