cmake_minimum_required(VERSION 3.20)
project(EncodingConverterWin32)

set(CMAKE_CXX_STANDARD 17)

# Source files
set(SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    resources.rc
    app.manifest
)

add_executable(EncodingConverterWin32 WIN32 ${SOURCES})

# Link Windows libraries
target_link_libraries(EncodingConverterWin32 PRIVATE
    comctl32
    uxtheme
    shell32
    ole32
    dwmapi
    Shcore
    Iconv::Iconv
    uchardet::libuchardet
)

# Include directories
target_include_directories(EncodingConverterWin32 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../common
)

target_compile_definitions(EncodingConverterWin32 PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    _WINDOWS
)

# Set subsystem to Windows and character set
set_target_properties(EncodingConverterWin32 PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_GLOBAL_CharacterSet "Unicode"
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    # Disable automatic manifest generation to avoid conflicts with our custom manifest
    VS_GLOBAL_EnableManagedIncrementalBuild "NO"
    VS_GLOBAL_GenerateManifest "FALSE"
)

# Specify our custom manifest file
set_target_properties(EncodingConverterWin32 PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /MANIFEST:NO"
)