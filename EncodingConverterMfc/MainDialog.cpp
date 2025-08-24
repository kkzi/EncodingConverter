#include "MainDialog.h"
#include "../common/FileConverter.hpp"
#include "stdafx.h"
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

IMPLEMENT_DYNAMIC(CMainDialog, CDialogEx)

BEGIN_MESSAGE_MAP(CMainDialog, CDialogEx)
ON_BN_CLICKED(IDC_BROWSE_DIR, &CMainDialog::OnBnClickedBrowseDir)
ON_BN_CLICKED(IDC_START_CONVERSION, &CMainDialog::OnBnClickedStartConversion)
ON_CBN_SELCHANGE(IDC_FILE_TYPE_COMBO, &CMainDialog::OnCbnSelchangeFileTypeCombo)
ON_BN_CLICKED(IDC_CLEAR_LOG, &CMainDialog::OnBnClickedClearLog)
ON_MESSAGE(WM_USER + 1, &CMainDialog::OnConversionComplete)
ON_WM_CTLCOLOR()
ON_WM_ERASEBKGND()
ON_WM_DRAWITEM()

END_MESSAGE_MAP()
CMainDialog::CMainDialog(CWnd *pParent)
    : CDialogEx(IDD_MAINDIALOG, pParent)
    , m_dirPath(_T(""))
    , m_fileExtensions(_T("*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx"))
    , m_targetEncoding(_T("UTF-8-BOM"))
    , m_createBackup(FALSE)
    , m_stopConversion(false)
{
}

CMainDialog::~CMainDialog()
{
    m_stopConversion = true;
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }
}

void CMainDialog::DoDataExchange(CDataExchange *pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DIR_PATH, m_dirPathEdit);
    DDX_Control(pDX, IDC_FILE_EXTENSIONS, m_fileExtensionsEdit);
    DDX_Control(pDX, IDC_TARGET_ENCODING, m_targetEncodingCombo);
    DDX_Control(pDX, IDC_FILE_TYPE_COMBO, m_fileTypeCombo);
    DDX_Control(pDX, IDC_CREATE_BACKUP, m_createBackupCheck);
    DDX_Control(pDX, IDC_START_CONVERSION, m_startConversionBtn);
    DDX_Control(pDX, IDC_PROGRESS, m_progressCtrl);
    DDX_Control(pDX, IDC_STATUS, m_statusText);
    DDX_Control(pDX, IDC_CLEAR_LOG, m_clearLogBtn);
    DDX_Control(pDX, IDC_LOG_LIST, m_logList);
    DDX_Text(pDX, IDC_DIR_PATH, m_dirPath);
    DDX_Text(pDX, IDC_FILE_EXTENSIONS, m_fileExtensions);
    DDX_Check(pDX, IDC_CREATE_BACKUP, m_createBackup);
}

BOOL CMainDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set window icon and title
    SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
    SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), FALSE);
    SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(0, 240, LWA_ALPHA);

    InitializeEncodings();
    InitializeFileTypes();

    m_fileExtensionsEdit.SetWindowText(m_fileExtensions);
    m_createBackupCheck.SetCheck(m_createBackup ? BST_CHECKED : BST_UNCHECKED);

    m_progressCtrl.SetRange(0, 100);
    m_progressCtrl.SetPos(0);

    // Force the RichEdit to redraw with custom border
    m_logList.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    m_logList.RedrawWindow(NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

    UpdateStatus(_T("Ready"));
    return TRUE;
}

void CMainDialog::InitializeEncodings()
{
    const TCHAR *encodings[] = { _T("UTF-8"), _T("UTF-8-BOM"), _T("GBK") };

    for (const auto &encoding : encodings)
    {
        m_targetEncodingCombo.AddString(encoding);
    }

    m_targetEncodingCombo.SetWindowText(m_targetEncoding);
}

void CMainDialog::OnBnClickedBrowseDir()
{
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
            pFileOpen->SetTitle(_T("Select a directory containing code files"));

            // Show the dialog
            hr = pFileOpen->Show(GetSafeHwnd());

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
                        m_dirPathEdit.SetWindowText(m_dirPath);
                        AddLogMessage(CString(_T("Selected: ")) + m_dirPath, RGB(0, 0, 0));
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

void CMainDialog::OnBnClickedStartConversion()
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

void CMainDialog::StopConversion()
{
    m_stopConversion = true;
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }
    AddLogMessage(_T("Stopping conversion..."), RGB(255, 140, 0));
    UpdateStatus(_T("Stopping..."));
}

void CMainDialog::StartConversion()
{
    // Check if a conversion is already running
    if (m_workerThread.joinable())
    {
        AddLogMessage(_T("Conversion already in progress"), RGB(255, 140, 0));
        return;
    }

    // Update data from controls
    UpdateData(TRUE);

    // Validate directory
    if (m_dirPath.IsEmpty())
    {
        AfxMessageBox(_T("Please select a directory first."), MB_ICONWARNING);
        return;
    }

    // Check if directory exists
    if (!fs::exists(fs::path(CStringToString(m_dirPath))))
    {
        AfxMessageBox(_T("The selected directory does not exist."), MB_ICONERROR);
        return;
    }

    // Get selected encoding
    int sel = m_targetEncodingCombo.GetCurSel();
    if (sel != CB_ERR)
    {
        CString encoding;
        m_targetEncodingCombo.GetLBText(sel, encoding);
        m_targetEncoding = encoding;
    }

    // Clear previous log
    m_logList.SetWindowText(_T(""));

    // Change button text and disable other controls during conversion
    m_startConversionBtn.SetWindowText(_T("Stop"));
    m_dirPathEdit.EnableWindow(FALSE);
    m_fileExtensionsEdit.EnableWindow(FALSE);
    m_targetEncodingCombo.EnableWindow(FALSE);
    m_createBackupCheck.EnableWindow(FALSE);

    // Reset progress
    m_progressCtrl.SetPos(0);
    UpdateStatus(_T("Scanning files..."));

    // Start worker thread
    m_stopConversion = false;
    m_workerThread = std::thread(&CMainDialog::ScanAndConvertFiles, this);
}

void CMainDialog::ScanAndConvertFiles()
{
    AddLogMessage(_T("Starting conversion..."), RGB(0, 0, 0));
    try
    {
        fs::path dirPath(CStringToString(m_dirPath));
        std::vector<fs::path> filesToConvert;

        // Parse file extensions
        std::vector<std::string> extensions;
        std::string extStr = CStringToString(m_fileExtensions);
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
        UpdateStatus(_T("Scanning for files..."));
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

        AddLogMessage(CString(_T("Found ")) + StringToCString(std::to_string(totalFiles)) + _T(" files"), RGB(0, 0, 0));

        // Convert files
        for (const auto &filepath : filesToConvert)
        {
            if (m_stopConversion)
                break;

            processedFiles++;
            UpdateProgress(processedFiles, totalFiles);
            UpdateStatus(CString(_T("Converting: ")) + StringToCString(filepath.filename().string()));

            try
            {
                std::string targetEncodingStr = CStringToString(m_targetEncoding);
                // Convert file using the ICU-based converter with detailed info
                ConversionInfo info = FileConverter::convertFileWithInfo(filepath, targetEncodingStr, m_createBackup);

                // Format log message: filename: old_encoding -> new_encoding [status]
                CString logMessage = StringToCString(filepath.filename().string()) + _T(": ");

                if (!info.sourceEncoding.empty())
                {
                    logMessage += StringToCString(info.sourceEncoding) + _T(" -> ") + StringToCString(info.targetEncoding);
                }
                else
                {
                    logMessage += _T("Unknown -> ") + StringToCString(info.targetEncoding);
                }

                COLORREF textColor = RGB(0, 0, 0);
                if (info.result == ConversionResult::Success)
                {
                    logMessage += _T(" [OK]");
                    textColor = RGB(0, 128, 0); // 绿色
                }
                else if (info.result == ConversionResult::AlreadyTargetEncoding)
                {
                    logMessage += _T(" [SKIP: ") + StringToCString(conversionResultToString(info.result));
                    if (!info.errorMessage.empty())
                    {
                        logMessage += _T(" - ") + StringToCString(info.errorMessage);
                    }
                    logMessage += _T("]");
                    textColor = RGB(255, 140, 0); // 橙色警告
                }
                else
                {
                    logMessage += _T(" [FAILED: ") + StringToCString(conversionResultToString(info.result));
                    if (!info.errorMessage.empty())
                    {
                        logMessage += _T(" - ") + StringToCString(info.errorMessage);
                    }
                    logMessage += _T("]");
                    textColor = RGB(220, 20, 60); // 红色错误
                }

                AddLogMessage(logMessage, textColor);
            }
            catch (const std::exception &e)
            {
                AddLogMessage(StringToCString(filepath.filename().string()) + _T(": [FAILED: ") + StringToCString(e.what()) + _T("]"), RGB(220, 20, 60));
            }
        }

        // Final update
        if (m_stopConversion)
        {
            UpdateStatus(_T("Conversion stopped"));
            AddLogMessage(_T("Conversion stopped"), RGB(255, 140, 0));
        }
        else
        {
            UpdateStatus(_T("Conversion completed"));
            AddLogMessage(CString(_T("Completed: ")) + StringToCString(std::to_string(processedFiles)) + _T(" files"), RGB(0, 128, 0));
        }

        UpdateProgress(totalFiles, totalFiles);
    }
    catch (const std::exception &e)
    {
        UpdateStatus(_T("Error during conversion"));
        AddLogMessage(CString(_T("Error: ")) + StringToCString(e.what()), RGB(220, 20, 60));
    }

    // Re-enable controls - use a custom message to re-enable controls
    PostMessage(WM_USER + 1, 0, 0);
}

void CMainDialog::AddLogMessage(const CString &message, COLORREF textColor)
{
    // Thread-safe UI update with mutex protection
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (::IsWindow(m_logList.GetSafeHwnd()))
    {
        int textLength = m_logList.GetWindowTextLength();
        m_logList.SetSel(textLength, textLength);
        
        CHARFORMAT cf;
        ZeroMemory(&cf, sizeof(CHARFORMAT));
        cf.cbSize = sizeof(CHARFORMAT);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = textColor;
        cf.dwEffects = 0;
        m_logList.SetSelectionCharFormat(cf);
        
        CString fullMessage = message + _T("\r\n");
        m_logList.ReplaceSel(fullMessage);
        
        m_logList.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
    }
}

void CMainDialog::UpdateProgress(int current, int total)
{
    if (total > 0 && ::IsWindow(m_progressCtrl.GetSafeHwnd()))
    {
        int percentage = (current * 100) / total;
        m_progressCtrl.SetPos(percentage);
    }
}

void CMainDialog::UpdateStatus(const CString &status)
{
    if (::IsWindow(m_statusText.GetSafeHwnd()))
    {
        m_statusText.SetWindowText(status);
    }
}

void CMainDialog::InitializeFileTypes()
{
    m_fileTypeCombo.AddString(_T("C/C++ Files"));
    m_fileTypeCombo.AddString(_T("Web Files"));
    m_fileTypeCombo.AddString(_T("Text Files"));
    // m_fileTypeCombo.AddString(_T("Custom"));

    m_fileTypeCombo.SetCurSel(0);  // Select C/C++ Files as default
}

void CMainDialog::OnCbnSelchangeFileTypeCombo()
{
    int sel = m_fileTypeCombo.GetCurSel();
    if (sel != CB_ERR)
    {
        switch (sel)
        {
        case 0:  // C/C++ Files
            SetFileExtensions(_T("*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx"));
            break;
        case 1:  // Web Files
            SetFileExtensions(_T("*.html;*.htm;*.css;*.js;*.php;*.asp;*.jsp"));
            break;
        case 2:  // Text Files
            SetFileExtensions(_T("*.txt;*.md;*.ini;*.cfg;*.conf;*.xml;*.json"));
            break;
        case 3:  // Custom
            break;
        }
    }
}

void CMainDialog::SetFileExtensions(const CString &extensions)
{
    m_fileExtensions = extensions;
    m_fileExtensionsEdit.SetWindowText(m_fileExtensions);
}

void CMainDialog::OnBnClickedClearLog()
{
    m_logList.SetWindowText(_T(""));
}

LRESULT CMainDialog::OnConversionComplete(WPARAM wParam, LPARAM lParam)
{
    // Wait for worker thread to complete and clean up
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }

    // Re-enable controls after conversion
    m_startConversionBtn.SetWindowText(_T("Start Conversion"));
    m_startConversionBtn.EnableWindow(TRUE);
    m_dirPathEdit.EnableWindow(TRUE);
    m_fileExtensionsEdit.EnableWindow(TRUE);
    m_targetEncodingCombo.EnableWindow(TRUE);
    m_createBackupCheck.EnableWindow(TRUE);
    return 0;
}

HBRUSH CMainDialog::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    pDC->SetBkColor(RGB(255, 255, 255));
    return (HBRUSH)GetStockObject(WHITE_BRUSH);
    // return hbr;
}
