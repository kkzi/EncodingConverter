#include "EncodingDialog.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>
#include <shellscalingapi.h>

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_PER_MONITOR_DPI_AWARE);
    EncodingDialog dialog;
    
    // 使用 windlg 的 runMain 函数来运行对话框
    return lib::runMain(dialog, hInstance, IDD_MAINDIALOG, nCmdShow, 1);
}