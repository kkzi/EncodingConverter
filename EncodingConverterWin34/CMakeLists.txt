cmake_minimum_required(VERSION 3.20)
project(EncodingConverterWin34 VERSION 1.0.0 LANGUAGES CXX)


# 添加 windlg 子目录
add_subdirectory(windlg)

# 添加可执行文件
add_executable(EncodingConverterWin34 WIN32)

# 添加源文件
target_sources(EncodingConverterWin34 PRIVATE
    main_new.cpp
    EncodingDialog.cpp
    EncodingDialog.h
    resource.h
    Dialog.rc
)

# 添加头文件目录
target_include_directories(EncodingConverterWin34 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/windlg
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

# 链接库
target_link_libraries(EncodingConverterWin34 PRIVATE
    windlg
    Iconv::Iconv
    uchardet::libuchardet
    comctl32
    shell32
    ole32
)

# 设置预处理器定义
target_compile_definitions(EncodingConverterWin34 PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# 编译选项
if(MSVC)
    target_compile_options(EncodingConverterWin34 PRIVATE
        /W3
        /MP
        $<$<CONFIG:Debug>:/bigobj>
    )
endif()


# 设置清单文件
if(MSVC)
    set_target_properties(EncodingConverterWin34 PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED"
    )
    # 添加清单文件作为源文件
    target_sources(EncodingConverterWin34 PRIVATE
        app.manifest
    )
endif()