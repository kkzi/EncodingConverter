#include "EncodingDialog.h"
#include <windowsx.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "windlg/CheckRadio.h"
#include "windlg/ComboBox.h"
#include "windlg/ListView.h"
#include "windlg/ProgressBar.h"

EncodingDialog::EncodingDialog()
    : m_fileExtensions(L"*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx")
    , m_targetEncoding(L"UTF-8-BOM")
    , m_createBackup(false)
    , m_isConverting(false)
    , m_stopConversion(false)
    , m_backgroundBrush(nullptr)
{
}

EncodingDialog::~EncodingDialog()
{
    m_stopConversion = true;
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }
    
    // Clean up background brush
    if (m_backgroundBrush != nullptr)
    {
        DeleteObject(m_backgroundBrush);
        m_backgroundBrush = nullptr;
    }
}

INT_PTR EncodingDialog::dlgProc(UINT uMsg, WPARAM wp, LPARAM lp)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitializeControls();
        InitializeEncodings();
        InitializeFileTypes();
        
        // Set default values
        lib::CheckRadio{this, IDC_CREATE_BACKUP_CHECK}.check(m_createBackup);
        lib::Window{GetDlgItem(hWnd(), IDC_STATUS_LABEL)}.setText(L"Ready");
        
        // Set dialog background to white
        if (m_backgroundBrush == nullptr) {
            m_backgroundBrush = CreateSolidBrush(RGB(255, 255, 255));
        }
        SetClassLongPtr(hWnd(), GCLP_HBRBACKGROUND, (LONG_PTR)m_backgroundBrush);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDC_BROWSE_DIR_BTN:
            if (HIWORD(wp) == BN_CLICKED)
                OnBrowseDir();
            break;

        case IDC_START_CONVERSION_BTN:
            if (HIWORD(wp) == BN_CLICKED)
                OnStartConversion();
            break;

        case IDC_FILE_TYPE_COMBO:
            if (HIWORD(wp) == CBN_SELCHANGE)
                OnFileTypeChanged();
            break;

        case IDC_CLEAR_LOG_BTN:
            if (HIWORD(wp) == BN_CLICKED)
                OnClearLog();
            break;
        }
        break;

    case WM_CLOSE:
        StopConversion();
        // Wait for worker thread to completely finish
        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
        // Destroy window instead of just ending dialog
        DestroyWindow(hWnd());
        return TRUE;
        
    case WM_DESTROY:
        // Send WM_QUIT message to message loop to ensure process exits
        PostQuitMessage(0);
        return TRUE;
        
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORDLG:
        {
            HDC hdc = (HDC)wp;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
    }
    return FALSE;
}

void EncodingDialog::InitializeControls()
{
}

void EncodingDialog::InitializeEncodings()
{
    lib::ComboBox combo{this, IDC_TARGET_ENC_COMBO};
    combo.clear();
    
    std::vector<std::wstring> encodings = {
        L"UTF-8",
        L"UTF-8-BOM",
        L"GBK",
        L"GB2312",
        L"UTF-16",
        L"UTF-32",
        L"ASCII"
    };

    for (const auto& encoding : encodings)
    {
        combo.add({encoding});
    }

    // Set default selection to UTF-8-BOM
    combo.select(1);
}

void EncodingDialog::InitializeFileTypes()
{
    lib::ComboBox combo{this, IDC_FILE_TYPE_COMBO};
    combo.clear();
    
    std::vector<std::wstring> fileTypes = {
        L"C/C++ Files",
        L"Web Files",
        L"Text Files"
    };

    for (const auto& fileType : fileTypes)
    {
        combo.add({fileType});
    }
    
    // Default select C/C++ Files
    combo.select(0);
}

void EncodingDialog::OnBrowseDir()
{
    auto selectedDir = dlg.showOpenFolder();
    if (selectedDir.has_value())
    {
        m_dirPath = selectedDir.value();
        lib::Window{GetDlgItem(hWnd(), IDC_DIR_PATH_EDIT)}.setText(m_dirPath);
        AddLogMessage(L"Selected: " + m_dirPath);
    }
}

void EncodingDialog::OnStartConversion()
{
    if (m_isConverting)
    {
        StopConversion();
    }
    else
    {
        StartConversion();
    }
}

void EncodingDialog::OnFileTypeChanged()
{
    lib::ComboBox combo{this, IDC_FILE_TYPE_COMBO};
    auto selIndex = combo.selectedIndex();
    
    if (selIndex.has_value())
    {
        std::wstring extensions;
        switch (selIndex.value())
        {
        case 0: // C/C++ Files
            extensions = L"*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx";
            break;
        case 1: // Web Files
            extensions = L"*.html;*.htm;*.css;*.js;*.php;*.asp;*.jsp";
            break;
        case 2: // Text Files
            extensions = L"*.txt;*.md;*.ini;*.cfg;*.conf;*.xml;*.json";
            break;
        }
        
        lib::Window{GetDlgItem(hWnd(), IDC_FILE_EXT_EDIT)}.setText(extensions);
    }
}

void EncodingDialog::OnClearLog()
{
    lib::ListView listView{this, IDC_LOG_LIST};
    listView.items.removeAll();
    AddLogMessage(L"Log cleared");
}

void EncodingDialog::StartConversion()
{
    // Check if already converting
    if (m_workerThread.joinable())
    {
        AddLogMessage(L"Conversion already in progress");
        return;
    }

    // Get directory path
    m_dirPath = lib::Window{GetDlgItem(hWnd(), IDC_DIR_PATH_EDIT)}.text();

    // Validate directory
    if (m_dirPath.empty())
    {
        dlg.msgBox(L"Warning", std::nullopt, L"Please select a directory first.", TDCBF_OK_BUTTON, TD_WARNING_ICON);
        return;
    }

    // Check if directory exists
    if (!fs::exists(fs::path(m_dirPath)))
    {
        dlg.msgBox(L"Error", std::nullopt, L"The selected directory does not exist.", TDCBF_OK_BUTTON, TD_ERROR_ICON);
        return;
    }

    // Get file extensions
    m_fileExtensions = lib::Window{GetDlgItem(hWnd(), IDC_FILE_EXT_EDIT)}.text();

    // Get selected encoding
    lib::ComboBox encCombo{this, IDC_TARGET_ENC_COMBO};
    auto selIndex = encCombo.selectedIndex();
    if (selIndex.has_value())
    {
        auto selText = encCombo.selectedText();
        if (selText.has_value())
        {
            m_targetEncoding = selText.value();
        }
    }

    // Get backup option
    m_createBackup = lib::CheckRadio{this, IDC_CREATE_BACKUP_CHECK}.isChecked();

    // Clear previous log
    lib::ListView listView{this, IDC_LOG_LIST};
    listView.items.removeAll();
    AddLogMessage(L"Starting conversion...");

    // Change button text and disable other controls during conversion
    lib::Window{GetDlgItem(hWnd(), IDC_START_CONVERSION_BTN)}.setText(L"Stop");
    dlg.enable({IDC_DIR_PATH_EDIT, IDC_FILE_EXT_EDIT, IDC_FILE_TYPE_COMBO, 
               IDC_TARGET_ENC_COMBO, IDC_CREATE_BACKUP_CHECK, IDC_BROWSE_DIR_BTN}, false);

    // Reset progress
    UpdateStatus(L"Scanning files...");
    lib::ProgressBar{this, IDC_PROGRESS_BAR}.setPos(0);

    // Start worker thread
    m_isConverting = true;
    m_stopConversion = false;
    m_workerThread = std::thread(&EncodingDialog::ScanAndConvertFiles, this);
}

void EncodingDialog::StopConversion()
{
    m_stopConversion = true;
    AddLogMessage(L"Stopping conversion...");
    UpdateStatus(L"Stopping...");
}

void EncodingDialog::ScanAndConvertFiles()
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

        // Scan files
        UpdateStatus(L"Scanning for files...");
        for (const auto& entry : fs::recursive_directory_iterator(dirPath))
        {
            if (m_stopConversion)
                break;

            if (fs::is_regular_file(entry.path()))
            {
                std::string filename = entry.path().filename().string();
                for (const auto& ext : extensions)
                {
                    if (ext.length() >= 2 && ext[0] == '*')
                    {
                        std::string pattern = ext.substr(1);
                        if (filename.length() >= pattern.length() &&
                            filename.substr(filename.length() - pattern.length()) == pattern)
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
        AddLogMessage(foundMsg);

        // Set progress bar range
        dlg.runUiThread([this, totalFiles]() {
            lib::ProgressBar{this, IDC_PROGRESS_BAR}.setRange(0, totalFiles);
        });

        // Convert files
        std::string targetEncodingStr = WStringToString(m_targetEncoding);
        for (const auto& filepath : filesToConvert)
        {
            if (m_stopConversion)
                break;

            processedFiles++;
            
            // Update progress in UI thread
            dlg.runUiThread([this, processedFiles]() {
                lib::ProgressBar{this, IDC_PROGRESS_BAR}.setPos(processedFiles);
            });

            std::wstring statusMsg = L"Converting: " + StringToWString(filepath.filename().string());
            UpdateStatus(statusMsg);

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
                
                AddLogMessage(logMsg);
            }
            catch (const std::exception& e)
            {
                std::wstring logMsg = StringToWString(filepath.filename().string()) + L": " + StringToWString(e.what());
                AddLogMessage(logMsg);
            }
        }

        // Final update
        if (m_stopConversion)
        {
            UpdateStatus(L"Conversion stopped");
            AddLogMessage(L"Conversion stopped");
        }
        else
        {
            UpdateStatus(L"Conversion completed");
            std::wstring completedMsg = L"Completed: " + std::to_wstring(processedFiles) + L" files";
            AddLogMessage(completedMsg);
        }

        // Re-enable controls in UI thread
        dlg.runUiThread([this]() {
            lib::Window{GetDlgItem(hWnd(), IDC_START_CONVERSION_BTN)}.setText(L"Start Conversion");
            dlg.enable({IDC_DIR_PATH_EDIT, IDC_FILE_EXT_EDIT, IDC_FILE_TYPE_COMBO, 
                       IDC_TARGET_ENC_COMBO, IDC_CREATE_BACKUP_CHECK, IDC_BROWSE_DIR_BTN}, true);
        });
    }
    catch (const std::exception& e)
    {
        UpdateStatus(L"Error during conversion");
        std::wstring errorMsg = L"Error: " + StringToWString(e.what());
        AddLogMessage(errorMsg);
    }

    m_isConverting = false;
}

void EncodingDialog::AddLogMessage(const std::wstring& message)
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    
    // Add log message in UI thread
    dlg.runUiThread([this, message]() {
        lib::ListView listView{this, IDC_LOG_LIST};
        listView.items.add(message);
        
        // Scroll to last item
        listView.items.removeAll(); // Clear selection first
        if (listView.items.count() > 0)
        {
            listView.items[listView.items.count() - 1].focus();
        }
    });
}

void EncodingDialog::UpdateProgress(int current, int total)
{
    // Update progress in UI thread
    dlg.runUiThread([this, current, total]() {
        lib::ProgressBar{this, IDC_PROGRESS_BAR}.setPos(current);
        
        std::wstring progressText = L"Progress: " + std::to_wstring(current) + L"/" + std::to_wstring(total);
        UpdateStatus(progressText);
    });
}

void EncodingDialog::UpdateStatus(const std::wstring& status)
{
    // Update status in UI thread
    dlg.runUiThread([this, status]() {
        lib::Window{GetDlgItem(hWnd(), IDC_STATUS_LABEL)}.setText(status);
    });
}

