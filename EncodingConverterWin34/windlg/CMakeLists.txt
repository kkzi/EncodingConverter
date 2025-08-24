cmake_minimum_required(VERSION 3.20)

# 创建 windlg 库
add_library(windlg STATIC)

# 添加所有源文件
target_sources(windlg PRIVATE
    CheckRadio.cpp
    CheckRadio.h
    Com.cpp
    Com.h
    ComboBox.cpp
    ComboBox.h
    CustomControl.cpp
    CustomControl.h
    Dialog.cpp
    Dialog.h
    DialogMain.cpp
    DialogMain.h
    DialogModal.cpp
    DialogModal.h
    dpi.cpp
    dpi.h
    File.cpp
    File.h
    ImgList.cpp
    ImgList.h
    ini.cpp
    ini.h
    lib.h
    ListView.cpp
    ListView.h
    Menu.cpp
    Menu.h
    NativeControl.cpp
    NativeControl.h
    path.cpp
    path.h
    ProgressBar.cpp
    ProgressBar.h
    StatusBar.cpp
    StatusBar.h
    str.cpp
    str.h
    TimeCount.cpp
    TimeCount.h
    vec.h
    VersionInfo.cpp
    VersionInfo.h
    Window.cpp
    Window.h
)

# 设置预处理器定义
target_compile_definitions(windlg PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# 链接系统库
target_link_libraries(windlg PRIVATE
    comctl32
    shell32
    ole32
    gdi32
    user32
    advapi32
)