cmake_minimum_required(VERSION 3.20)
project(EncodingConverterMfc)

set(CMAKE_CXX_STANDARD 17)

# Find MFC and ICU
find_package(MFC REQUIRED)
set(CMAKE_MFC_FLAG 2)

# Source files
set(SOURCES
    main.cpp
    MainDialog.cpp
    MainDialog.h
    CustomRichEdit.cpp
    CustomRichEdit.h
    resource.h
    stdafx.cpp
    stdafx.h
    targetver.h
    Dialog.rc
    app.manifest
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Iconv::Iconv uchardet::libuchardet
    comctl32
    uxtheme
    Version.lib
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common
    ${ICU_INCLUDE_DIRS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    _AFXDLL
    _WINDOWS
    U_STATIC_IMPLEMENTATION
)

# Set subsystem to Windows and character set
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_GLOBAL_CharacterSet "Unicode"
    # MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup /NODEFAULTLIB:LIBCMT.lib"
)
