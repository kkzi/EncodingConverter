#include "MainWindow.h"
#include "../common/FileConverter.hpp"
#include "../common/StringUtils.hpp"
#include <commctrl.h>
#include <iostream>
#include <shobjidl.h>
#include <windowsx.h>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

MainWindow::MainWindow()
    : m_hInstance(nullptr)
    , m_hwnd(nullptr)
    , m_hFont(nullptr)
    , m_hDirPathEdit(nullptr)
    , m_hBrowseDirBtn(nullptr)
    , m_hFileExtensionsEdit(nullptr)
    , m_hTargetEncodingCombo(nullptr)
    , m_hFileTypeCombo(nullptr)
    , m_hCreateBackupCheck(nullptr)
    , m_hStartConversionBtn(nullptr)
    , m_hProgressCtrl(nullptr)
    , m_hStatusText(nullptr)
    , m_hClearLogBtn(nullptr)
    , m_hLogList(nullptr)
    , m_fileExtensions(L"*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx")
    , m_targetEncoding(L"UTF-8-BOM")
    , m_createBackup(false)
    , m_stopConversion(false)
    , m_dpiScale(1.0f)
    , m_dpi(96)
{
    InitializeDPI();
}

MainWindow::~MainWindow()
{
    m_stopConversion = true;
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }

    if (m_hFont)
    {
        DeleteObject(m_hFont);
    }
    if (m_hBackgroundBrush)
    {
        DeleteObject(m_hBackgroundBrush);
    }
}

bool MainWindow::Create(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    // Register window class with modern style
    const wchar_t CLASS_NAME[] = L"EncodingConverterWindow";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  // Modern white background
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));    // Load app.ico icon
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(1));  // Small icon

    RegisterClassEx(&wc);

    // Create window with modern appearance
    // Scale initial window size for DPI
    int scaledWidth = ScaleForDPI(790);
    int scaledHeight = ScaleForDPI(410);

    // Calculate window position to center it on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - scaledWidth) / 2;
    int windowY = (screenHeight - scaledHeight) / 2;

    m_hwnd = CreateWindowEx(WS_EX_LAYERED,  // Enable layered window for better visual effects
        CLASS_NAME, L"File Encoding Converter", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, windowX,
        windowY,  // Position window in center of screen
        scaledWidth, scaledHeight, nullptr, nullptr, hInstance, this);

    if (m_hwnd)
    {
        // Set window icon
        HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
        SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

        // Enable modern visual styles
        SetWindowTheme(m_hwnd, L"Explorer", nullptr);

        // Extend frame for shadow effect
        MARGINS margins = { 0, 0, 0, 1 };  // Minimal margin for shadow
        DwmExtendFrameIntoClientArea(m_hwnd, &margins);

        // Set subtle transparency for modern look
        SetLayeredWindowAttributes(m_hwnd, 0, 245, LWA_ALPHA);
    }

    return m_hwnd != nullptr;
}

void MainWindow::Show(int nCmdShow)
{
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MainWindow *pThis = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
        pThis = (MainWindow *)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    }
    else
    {
        pThis = (MainWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis)
    {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        OnCreate();
        return 0;

    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_COMMAND:
        OnCommand(wParam, lParam);
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(0, 0, 0));  // Black text
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)m_hBackgroundBrush;
    }

    case WM_DPICHANGED:
        OnDpiChanged(wParam, lParam);
        return 0;

    case WM_CONVERSION_COMPLETE:
        OnConversionComplete();
        return 0;

    case WM_UPDATE_PROGRESS:
        OnUpdateProgress(wParam, lParam);
        return 0;

    case WM_UPDATE_STATUS:
        OnUpdateStatus(lParam);
        return 0;

    case WM_ADD_LOG:
        OnAddLog(lParam);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void MainWindow::OnSize(int width, int height)
{
    if (!m_hwnd)
        return;

    // Fixed dimensions with DPI scaling
    const int margin = ScaleForDPI(10);
    const int controlHeight = ScaleForDPI(23);
    const int labelWidth = ScaleForDPI(100);
    const int spacing = ScaleForDPI(12);
    const int contentWidth = width - (2 * margin);
    int yPos = margin;

    // Directory path section
    SetWindowPos(m_hDirPathEdit, nullptr, margin + labelWidth, yPos, contentWidth - labelWidth - ScaleForDPI(110), controlHeight, SWP_NOZORDER);
    SetWindowPos(m_hBrowseDirBtn, nullptr, width - margin - ScaleForDPI(95), yPos, ScaleForDPI(95), controlHeight, SWP_NOZORDER);
    yPos += controlHeight + spacing;

    // File extensions section
    SetWindowPos(m_hFileExtensionsEdit, nullptr, margin + labelWidth, yPos, contentWidth - labelWidth - ScaleForDPI(110), controlHeight, SWP_NOZORDER);
    SetWindowPos(m_hFileTypeCombo, nullptr, width - margin - ScaleForDPI(95), yPos, ScaleForDPI(95), ScaleForDPI(120), SWP_NOZORDER);
    yPos += controlHeight + spacing;

    // Target encoding section
    SetWindowPos(m_hTargetEncodingCombo, nullptr, margin + labelWidth, yPos, contentWidth - labelWidth - ScaleForDPI(110), ScaleForDPI(120), SWP_NOZORDER);
    yPos += controlHeight + spacing + ScaleForDPI(5);

    // Action buttons section
    SetWindowPos(m_hStartConversionBtn, nullptr, margin, yPos, ScaleForDPI(120), ScaleForDPI(28), SWP_NOZORDER);
    SetWindowPos(m_hCreateBackupCheck, nullptr, ScaleForDPI(160), yPos + ScaleForDPI(5), ScaleForDPI(150), ScaleForDPI(18), SWP_NOZORDER);
    yPos += ScaleForDPI(35);

    // Log list
    SetWindowPos(m_hLogList, nullptr, margin, yPos, contentWidth, ScaleForDPI(180), SWP_NOZORDER);
    yPos += ScaleForDPI(187);

    // Status section
    SetWindowPos(m_hProgressCtrl, nullptr, margin, yPos, ScaleForDPI(120), ScaleForDPI(18), SWP_NOZORDER);
    SetWindowPos(m_hStatusText, nullptr, ScaleForDPI(150), yPos + ScaleForDPI(1), contentWidth - ScaleForDPI(270), ScaleForDPI(16), SWP_NOZORDER);
    SetWindowPos(m_hClearLogBtn, nullptr, width - margin - ScaleForDPI(70), yPos - ScaleForDPI(3), ScaleForDPI(70), controlHeight, SWP_NOZORDER);
}

void MainWindow::OnCreate()
{
    // Create fonts with DPI scaling
    UpdateDpiDependentResources();

    m_hBackgroundBrush = CreateSolidBrush(RGB(255, 255, 255));  // White background

    InitializeControls();
    InitializeEncodings();
    InitializeFileTypes();

    // Set default values
    SetWindowTextW(m_hFileExtensionsEdit, m_fileExtensions.c_str());
    Button_SetCheck(m_hCreateBackupCheck, m_createBackup ? BST_CHECKED : BST_UNCHECKED);
    SetWindowTextW(m_hStatusText, L"Ready");

    // Initial layout
    RECT rect;
    GetClientRect(m_hwnd, &rect);
    OnSize(rect.right - rect.left, rect.bottom - rect.top);
}

void MainWindow::InitializeControls()
{
    // Fixed dimensions with DPI scaling
    const int margin = ScaleForDPI(10);
    const int controlHeight = ScaleForDPI(23);
    const int labelWidth = ScaleForDPI(100);
    const int spacing = ScaleForDPI(12);
    const int windowWidth = ScaleForDPI(790);
    const int contentWidth = windowWidth - (2 * margin);
    int yPos = margin;

    // Directory path section - label and controls on same line
    HWND hLabelDir = CreateWindow(L"STATIC", L"Code Directory:", WS_VISIBLE | WS_CHILD | SS_LEFT, margin, yPos + ScaleForDPI(3), labelWidth, controlHeight,
        m_hwnd, nullptr, m_hInstance, nullptr);
    SendMessage(hLabelDir, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    m_hDirPathEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, margin + labelWidth - ScaleForDPI(10), yPos,
        contentWidth - labelWidth - ScaleForDPI(50), controlHeight, m_hwnd, (HMENU)IDC_DIR_PATH, m_hInstance, nullptr);
    SendMessage(m_hDirPathEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    m_hBrowseDirBtn = CreateWindow(L"BUTTON", L"Browse...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, windowWidth - margin - ScaleForDPI(80), yPos,
        ScaleForDPI(80), controlHeight, m_hwnd, (HMENU)IDC_BROWSE_DIR, m_hInstance, nullptr);
    SendMessage(m_hBrowseDirBtn, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    yPos += controlHeight + spacing;

    // File extensions section - label and controls on same line
    HWND hLabelExt = CreateWindow(L"STATIC", L"File Extensions:", WS_VISIBLE | WS_CHILD | SS_LEFT, margin, yPos + ScaleForDPI(3), labelWidth, controlHeight,
        m_hwnd, nullptr, m_hInstance, nullptr);
    SendMessage(hLabelExt, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    m_hFileExtensionsEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, margin + labelWidth, yPos,
        contentWidth - labelWidth - 110, controlHeight, m_hwnd, (HMENU)IDC_FILE_EXTENSIONS, m_hInstance, nullptr);
    SendMessage(m_hFileExtensionsEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    m_hFileTypeCombo = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, windowWidth - margin - ScaleForDPI(95), yPos,
        ScaleForDPI(95), ScaleForDPI(120), m_hwnd, (HMENU)IDC_FILE_TYPE_COMBO, m_hInstance, nullptr);
    SendMessage(m_hFileTypeCombo, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    yPos += controlHeight + spacing;

    // Target encoding section - label and controls on same line
    HWND hLabelEnc = CreateWindow(L"STATIC", L"Target Encoding:", WS_VISIBLE | WS_CHILD | SS_LEFT, margin, yPos + ScaleForDPI(3), labelWidth, controlHeight,
        m_hwnd, nullptr, m_hInstance, nullptr);
    SendMessage(hLabelEnc, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    m_hTargetEncodingCombo = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, margin + labelWidth, yPos,
        contentWidth - labelWidth - ScaleForDPI(110), ScaleForDPI(120), m_hwnd, (HMENU)IDC_TARGET_ENCODING, m_hInstance, nullptr);
    SendMessage(m_hTargetEncodingCombo, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    yPos += controlHeight + spacing + ScaleForDPI(5);

    // Action buttons section
    m_hStartConversionBtn = CreateWindow(L"BUTTON", L"Start Conversion", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, margin, yPos, ScaleForDPI(120), ScaleForDPI(28),
        m_hwnd, (HMENU)IDC_START_CONVERSION, m_hInstance, nullptr);
    SendMessage(m_hStartConversionBtn, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    m_hCreateBackupCheck = CreateWindow(L"BUTTON", L"Create Backup Files", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, ScaleForDPI(160), yPos + ScaleForDPI(5),
        ScaleForDPI(150), ScaleForDPI(18), m_hwnd, (HMENU)IDC_CREATE_BACKUP, m_hInstance, nullptr);
    SendMessage(m_hCreateBackupCheck, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    yPos += ScaleForDPI(35);

    // Log list (removed "Conversion Log" label)
    m_hLogList = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT, margin, yPos,
        contentWidth, ScaleForDPI(180), m_hwnd, (HMENU)IDC_LOG_LIST, m_hInstance, nullptr);
    SendMessage(m_hLogList, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // Set background color to match window
    SendMessage(m_hLogList, LB_SETITEMDATA, 0, (LPARAM)m_hBackgroundBrush);
    yPos += ScaleForDPI(185);  // Reduced bottom padding

    // Status section
    m_hProgressCtrl = CreateWindow(PROGRESS_CLASS, L"", WS_VISIBLE | WS_CHILD | PBS_SMOOTH, margin, yPos, ScaleForDPI(120), ScaleForDPI(18), m_hwnd,
        (HMENU)IDC_PROGRESS, m_hInstance, nullptr);

    m_hStatusText = CreateWindow(L"STATIC", L"Ready", WS_VISIBLE | WS_CHILD | SS_LEFT, ScaleForDPI(150), yPos + ScaleForDPI(1), contentWidth - ScaleForDPI(270),
        ScaleForDPI(16), m_hwnd, (HMENU)IDC_STATUS, m_hInstance, nullptr);
    SendMessage(m_hStatusText, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // Clear Log button moved to status line, right-aligned with log list
    m_hClearLogBtn = CreateWindow(L"BUTTON", L"Clear Log", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, windowWidth - margin - ScaleForDPI(70), yPos - ScaleForDPI(3),
        ScaleForDPI(70), controlHeight, m_hwnd, (HMENU)IDC_CLEAR_LOG, m_hInstance, nullptr);
    SendMessage(m_hClearLogBtn, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // Setup progress bar
    SendMessage(m_hProgressCtrl, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(m_hProgressCtrl, PBM_SETPOS, 0, 0);
}

void MainWindow::InitializeEncodings()
{
    const wchar_t *encodings[] = { L"UTF-8", L"UTF-8-BOM", L"GBK", L"GB2312", L"UTF-16", L"UTF-32", L"ASCII" };

    for (const auto &encoding : encodings)
    {
        ComboBox_AddString(m_hTargetEncodingCombo, encoding);
    }

    ComboBox_SetCurSel(m_hTargetEncodingCombo, 1);  // Select UTF-8-BOM as default
}

void MainWindow::InitializeFileTypes()
{
    ComboBox_AddString(m_hFileTypeCombo, L"C/C++ Files");
    ComboBox_AddString(m_hFileTypeCombo, L"Web Files");
    ComboBox_AddString(m_hFileTypeCombo, L"Text Files");

    ComboBox_SetCurSel(m_hFileTypeCombo, 0);  // Select C/C++ Files as default
}

void MainWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    switch (wmId)
    {
    case IDC_BROWSE_DIR:
        OnBrowseDir();
        break;

    case IDC_START_CONVERSION:
        OnStartConversion();
        break;

    case IDC_FILE_TYPE_COMBO:
        if (wmEvent == CBN_SELCHANGE)
        {
            OnFileTypeChange();
        }
        break;

    case IDC_CLEAR_LOG:
        OnClearLog();
        break;
    }
}

void MainWindow::OnBrowseDir()
{
    BrowseForDirectory();
}

void MainWindow::BrowseForDirectory()
{
    // Use modern IFileOpenDialog interface (Windows Vista and later)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog *pFileOpen = nullptr;

        // Create the FileOpenDialog object
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Set the options on the dialog
            DWORD dwOptions;
            if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions)))
            {
                pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
            }

            // Set the title
            pFileOpen->SetTitle(L"Select a directory containing code files");

            // Show the dialog
            hr = pFileOpen->Show(m_hwnd);

            if (SUCCEEDED(hr))
            {
                // Get the selection from the user
                IShellItem *pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    if (SUCCEEDED(hr))
                    {
                        m_dirPath = pszFilePath;
                        SetWindowText(m_hDirPathEdit, m_dirPath.c_str());
                        AddLogMessage(std::wstring(L"Selected: ") + m_dirPath);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
}

void MainWindow::OnStartConversion()
{
    if (m_workerThread.joinable())
    {
        // If conversion is running, stop it
        StopConversion();
    }
    else
    {
        // If no conversion is running, start it
        StartConversion();
    }
}

void MainWindow::StopConversion()
{
    m_stopConversion = true;
    AddLogMessage(L"Stopping conversion...");
    UpdateStatus(L"Stopping...");
}

void MainWindow::StartConversion()
{
    // Check if a conversion is already running
    if (m_workerThread.joinable())
    {
        AddLogMessage(L"Conversion already in progress");
        return;
    }

    // Get directory path
    wchar_t dirPath[MAX_PATH];
    GetWindowText(m_hDirPathEdit, dirPath, MAX_PATH);
    m_dirPath = dirPath;

    // Validate directory
    if (m_dirPath.empty())
    {
        MessageBox(m_hwnd, L"Please select a directory first.", L"Warning", MB_ICONWARNING);
        return;
    }

    // Check if directory exists
    if (!fs::exists(fs::path(m_dirPath)))
    {
        MessageBox(m_hwnd, L"The selected directory does not exist.", L"Error", MB_ICONERROR);
        return;
    }

    // Get file extensions
    wchar_t fileExts[1024];
    GetWindowText(m_hFileExtensionsEdit, fileExts, 1024);
    m_fileExtensions = fileExts;

    // Get selected encoding
    int sel = ComboBox_GetCurSel(m_hTargetEncodingCombo);
    if (sel != CB_ERR)
    {
        wchar_t encoding[256];
        ComboBox_GetLBText(m_hTargetEncodingCombo, sel, encoding);
        m_targetEncoding = encoding;
    }

    // Get backup option
    m_createBackup = (Button_GetCheck(m_hCreateBackupCheck) == BST_CHECKED);

    // Clear previous log
    ListBox_ResetContent(m_hLogList);
    AddLogMessage(L"Starting conversion...");

    // Change button text and disable other controls during conversion
    SetWindowText(m_hStartConversionBtn, L"Stop");
    EnableWindow(m_hDirPathEdit, FALSE);
    EnableWindow(m_hFileExtensionsEdit, FALSE);
    EnableWindow(m_hTargetEncodingCombo, FALSE);
    EnableWindow(m_hCreateBackupCheck, FALSE);

    // Reset progress
    SendMessage(m_hProgressCtrl, PBM_SETPOS, 0, 0);
    UpdateStatus(L"Scanning files...");

    // Start worker thread
    m_stopConversion = false;
    m_workerThread = std::thread(&MainWindow::ScanAndConvertFiles, this);
}

void MainWindow::ScanAndConvertFiles()
{
    try
    {
        fs::path dirPath(m_dirPath);
        std::vector<fs::path> filesToConvert;

        // Parse file extensions
        std::vector<std::string> extensions;
        std::string extStr = WStringToString(m_fileExtensions);
        size_t pos = 0;
        size_t found = extStr.find(';');
        while (found != std::string::npos)
        {
            std::string ext = extStr.substr(pos, found - pos);
            if (!ext.empty())
            {
                extensions.push_back(ext);
            }
            pos = found + 1;
            found = extStr.find(';', pos);
        }
        if (pos < extStr.length())
        {
            std::string ext = extStr.substr(pos);
            if (!ext.empty())
            {
                extensions.push_back(ext);
            }
        }

        // Scan for files
        PostMessage(m_hwnd, WM_UPDATE_STATUS, 0, (LPARAM) new std::wstring(L"Scanning for files..."));
        for (const auto &entry : fs::recursive_directory_iterator(dirPath))
        {
            if (m_stopConversion)
                break;

            if (fs::is_regular_file(entry.path()))
            {
                std::string filename = entry.path().filename().string();
                for (const auto &ext : extensions)
                {
                    if (ext.length() >= 2 && ext[0] == '*')
                    {
                        std::string pattern = ext.substr(1);
                        if (filename.length() >= pattern.length() && filename.substr(filename.length() - pattern.length()) == pattern)
                        {
                            filesToConvert.push_back(entry.path());
                            break;
                        }
                    }
                }
            }
        }

        int totalFiles = static_cast<int>(filesToConvert.size());
        int processedFiles = 0;

        std::wstring foundMsg = L"Found " + std::to_wstring(totalFiles) + L" files";
        PostMessage(m_hwnd, WM_ADD_LOG, 0, (LPARAM) new std::wstring(foundMsg));

        // Convert files
        std::string targetEncodingStr = WStringToString(m_targetEncoding);
        for (const auto &filepath : filesToConvert)
        {
            if (m_stopConversion)
                break;

            processedFiles++;
            PostMessage(m_hwnd, WM_UPDATE_PROGRESS, processedFiles, totalFiles);
            std::wstring statusMsg = L"Converting: " + StringToWString(filepath.filename().string());
            PostMessage(m_hwnd, WM_UPDATE_STATUS, 0, (LPARAM) new std::wstring(statusMsg));

            try
            {
                // Convert file encoding using the new convertFileWithInfo function
                ConversionInfo info = FileConverter::convertFileWithInfo(filepath, targetEncodingStr, m_createBackup);

                std::wstring logMsg = StringToWString(filepath.filename().string()) + L": ";

                if (!info.sourceEncoding.empty())
                {
                    logMsg += StringToWString(info.sourceEncoding) + L" -> " + StringToWString(info.targetEncoding);
                }
                else
                {
                    logMsg += L"Unknown -> " + StringToWString(info.targetEncoding);
                }

                if (info.result == ConversionResult::Success)
                {
                    logMsg += L" [OK]";
                }
                else if (info.result == ConversionResult::AlreadyTargetEncoding)
                {
                    logMsg += L" [SKIP: " + StringToWString(conversionResultToString(info.result));
                    if (!info.errorMessage.empty())
                    {
                        logMsg += L" - " + StringToWString(info.errorMessage);
                    }
                    logMsg += L"]";
                }
                else
                {
                    logMsg += L" [FAILED: " + StringToWString(conversionResultToString(info.result));
                    if (!info.errorMessage.empty())
                    {
                        logMsg += L" - " + StringToWString(info.errorMessage);
                    }
                    logMsg += L"]";
                }

                PostMessage(m_hwnd, WM_ADD_LOG, 0, (LPARAM) new std::wstring(logMsg));
            }
            catch (const std::exception &e)
            {
                std::wstring logMsg = StringToWString(filepath.filename().string()) + L": " + StringToWString(e.what());
                PostMessage(m_hwnd, WM_ADD_LOG, 0, (LPARAM) new std::wstring(logMsg));
            }
        }

        // Final update
        if (m_stopConversion)
        {
            PostMessage(m_hwnd, WM_UPDATE_STATUS, 0, (LPARAM) new std::wstring(L"Conversion stopped"));
            PostMessage(m_hwnd, WM_ADD_LOG, 0, (LPARAM) new std::wstring(L"Conversion stopped"));
        }
        else
        {
            PostMessage(m_hwnd, WM_UPDATE_STATUS, 0, (LPARAM) new std::wstring(L"Conversion completed"));
            std::wstring completedMsg = L"Completed: " + std::to_wstring(processedFiles) + L" files";
            PostMessage(m_hwnd, WM_ADD_LOG, 0, (LPARAM) new std::wstring(completedMsg));
        }

        PostMessage(m_hwnd, WM_UPDATE_PROGRESS, totalFiles, totalFiles);
    }
    catch (const std::exception &e)
    {
        PostMessage(m_hwnd, WM_UPDATE_STATUS, 0, (LPARAM) new std::wstring(L"Error during conversion"));
        std::wstring errorMsg = L"Error: " + StringToWString(e.what());
        PostMessage(m_hwnd, WM_ADD_LOG, 0, (LPARAM) new std::wstring(errorMsg));
    }

    // Re-enable controls
    PostMessage(m_hwnd, WM_CONVERSION_COMPLETE, 0, 0);
}

void MainWindow::OnFileTypeChange()
{
    int sel = ComboBox_GetCurSel(m_hFileTypeCombo);
    if (sel != CB_ERR)
    {
        switch (sel)
        {
        case 0:  // C/C++ Files
            SetFileExtensions(L"*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx");
            break;
        case 1:  // Web Files
            SetFileExtensions(L"*.html;*.htm;*.css;*.js;*.php;*.asp;*.jsp");
            break;
        case 2:  // Text Files
            SetFileExtensions(L"*.txt;*.md;*.ini;*.cfg;*.conf;*.xml;*.json");
            break;
        }
    }
}

void MainWindow::SetFileExtensions(const std::wstring &extensions)
{
    m_fileExtensions = extensions;
    SetWindowText(m_hFileExtensionsEdit, m_fileExtensions.c_str());
    AddLogMessage(std::wstring(L"Extensions: ") + extensions);
}

void MainWindow::OnClearLog()
{
    ListBox_ResetContent(m_hLogList);
    AddLogMessage(L"Log cleared");
}

void MainWindow::OnConversionComplete()
{
    // Wait for worker thread to complete and clean up
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }

    // Re-enable controls after conversion
    SetWindowText(m_hStartConversionBtn, L"Start Conversion");
    EnableWindow(m_hStartConversionBtn, TRUE);
    EnableWindow(m_hDirPathEdit, TRUE);
    EnableWindow(m_hFileExtensionsEdit, TRUE);
    EnableWindow(m_hTargetEncodingCombo, TRUE);
    EnableWindow(m_hCreateBackupCheck, TRUE);
}

void MainWindow::OnUpdateProgress(WPARAM wParam, LPARAM lParam)
{
    int current = static_cast<int>(wParam);
    int total = static_cast<int>(lParam);
    UpdateProgress(current, total);
}

void MainWindow::OnUpdateStatus(LPARAM lParam)
{
    std::wstring *status = reinterpret_cast<std::wstring *>(lParam);
    if (status)
    {
        UpdateStatus(*status);
        delete status;
    }
}

void MainWindow::OnAddLog(LPARAM lParam)
{
    std::wstring *message = reinterpret_cast<std::wstring *>(lParam);
    if (message)
    {
        AddLogMessage(*message);
        delete message;
    }
}

void MainWindow::AddLogMessage(const std::wstring &message)
{
    std::lock_guard<std::mutex> lock(m_logMutex);

    // Check if the window is still valid before accessing UI controls
    if (IsWindow(m_hLogList))
    {
        ListBox_AddString(m_hLogList, message.c_str());
        int count = ListBox_GetCount(m_hLogList);
        ListBox_SetCurSel(m_hLogList, count - 1);
    }
}

void MainWindow::UpdateProgress(int current, int total)
{
    if (total > 0 && IsWindow(m_hProgressCtrl))
    {
        int percentage = (current * 100) / total;
        SendMessage(m_hProgressCtrl, PBM_SETPOS, percentage, 0);
    }
}

void MainWindow::UpdateStatus(const std::wstring &status)
{
    if (IsWindow(m_hStatusText))
    {
        SetWindowTextW(m_hStatusText, status.c_str());
    }
}

// Use FileConverter for encoding detection
std::string MainWindow::DetectFileEncoding(const fs::path &filepath)
{
    try
    {
        // Use the public interface to get encoding info
        ConversionInfo info = FileConverter::convertFileWithInfo(filepath, "UTF-8", false);
        return info.sourceEncoding;
    }
    catch (const std::exception &e)
    {
        std::wcout << L"Error detecting encoding: " << StringToWString(e.what()) << std::endl;
        return "";
    }
}

// Use FileConverter for encoding conversion
bool MainWindow::ConvertFileEncoding(const fs::path &filepath, const std::string &targetEncoding)
{
    try
    {
        // Use the public interface for conversion
        ConversionInfo info = FileConverter::convertFileWithInfo(filepath, targetEncoding, false);
        return info.result == ConversionResult::Success || info.result == ConversionResult::AlreadyTargetEncoding;
    }
    catch (const std::exception &e)
    {
        std::wcout << L"Error converting file: " << StringToWString(e.what()) << std::endl;
    }

    return false;
}

// DPI support implementation
void MainWindow::InitializeDPI()
{
    // Get DPI awareness context
    DPI_AWARENESS_CONTEXT context = GetThreadDpiAwarenessContext();
    DPI_AWARENESS awareness = GetAwarenessFromDpiAwarenessContext(context);

    // Get the DPI for the primary monitor
    m_dpi = GetDpiForWindow(m_hwnd);
    if (m_dpi == 0)
    {
        // Fallback to desktop DC method
        HDC hdc = GetDC(nullptr);
        m_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
    }

    // Calculate DPI scale factor
    m_dpiScale = static_cast<float>(m_dpi) / 96.0f;
}

float MainWindow::GetDPIScale() const
{
    return m_dpiScale;
}

int MainWindow::ScaleForDPI(int value) const
{
    return static_cast<int>(value * m_dpiScale + 0.5f);
}

void MainWindow::UpdateDpiDependentResources()
{
    // Delete existing fonts
    if (m_hFont)
    {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
    }

    int fontSize = ScaleForDPI(14);

    m_hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Update fonts for all controls
    if (m_hwnd)
    {
        EnumChildWindows(
            m_hwnd,
            [](HWND hwnd, LPARAM lParam) -> BOOL {
                MainWindow *pThis = reinterpret_cast<MainWindow *>(lParam);
                if (pThis && pThis->m_hFont)
                {
                    SendMessage(hwnd, WM_SETFONT, (WPARAM)pThis->m_hFont, TRUE);
                }
                return TRUE;
            },
            (LPARAM)this);
    }

    // Relayout the window
    if (m_hwnd)
    {
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        OnSize(rect.right - rect.left, rect.bottom - rect.top);
    }
}

void MainWindow::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
    // Update DPI values
    m_dpi = static_cast<UINT>(LOWORD(wParam));
    m_dpiScale = static_cast<float>(m_dpi) / 96.0f;

    // Update DPI-dependent resources
    UpdateDpiDependentResources();

    // Resize window if needed
    RECT *const prcNewWindow = reinterpret_cast<RECT *>(lParam);
    if (prcNewWindow)
    {
        SetWindowPos(m_hwnd, nullptr, prcNewWindow->left, prcNewWindow->top, prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
