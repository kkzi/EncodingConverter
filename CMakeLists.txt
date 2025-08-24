cmake_minimum_required(VERSION 3.21)
project(EncodingConv)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable MFC support
set(CMAKE_MFC_FLAG 2)  # 1=static MFC, 2=shared DLL MFC

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/lib)

# Find required packages
find_package(Iconv REQUIRED)
find_package(uchardet CONFIG REQUIRED)

# Set MFC and ATL support
set(CMAKE_WIN32_EXECUTABLE TRUE)
set(CMAKE_MFC_FLAG 2)  # Use MFC in a shared DLL
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Common MFC compiler definitions
add_definitions(
    -D_WINDOWS
    -D_AFXDLL
    -D_AFXEXT
    -D_WINDLL
    -D_USRDLL
    -D_UNICODE
    -DUNICODE
    -DWIN32_LEAN_AND_MEAN
    -DNOMINMAX
    -D_WIN32_WINNT=0x0A00   # Windows 10
    -DNTDDI_VERSION=0x0A000000
    -D_WIN32_IE=0x0A00
)

# Add subdirectories
# add_subdirectory(cli)

if(VCPKG_TARGET_TRIPLET STREQUAL "x64-windows-static-md")
    add_subdirectory(EncodingConverterMfc)
    add_subdirectory(EncodingConverterWin32)
    add_subdirectory(EncodingConverterWin34)
    # add_subdirectory(EncodingConverterWin35)
else()
    # add_subdirectory(EncodingConverter)
endif()