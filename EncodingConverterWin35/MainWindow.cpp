#define WIN32_LEAN_AND_MEAN
#include "MainWindow.h"
#include "../common/FileConverter.hpp"
#include <shlobj.h>
#include <windows.h>

using namespace vaca;

MainWindow::MainWindow()
    : vaca::Frame(L"File Encoding Converter")
    , m_dirPathLabel(L"Code Directory:", this)
    , m_dirPathEdit(L"", this)
    , m_browseDirBtn(L"Browse...", this)
    , m_fileExtensionsLabel(L"File Extensions:", this)
    , m_fileExtensionsEdit(L"*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx", this)
    , m_fileTypeCombo(this)
    , m_targetEncodingLabel(L"Target Encoding:", this)
    , m_targetEncodingCombo(this)
    , m_startConversionBtn(L"Start Conversion", this)
    , m_createBackupCheck(L"Create Backup Files", this)
    , m_logList(this)
    , m_progressCtrl(this)
    , m_statusText(L"Ready", this)
    , m_clearLogBtn(L"Clear Log", this)
    , m_fileExtensions(L"*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx")
    , m_targetEncoding(L"UTF-8-BOM")
    , m_createBackup(false)
    , m_stopConversion(false)
{
    auto rowHeight = 24;
    auto padding = 10;
    auto y = padding + 5;
    m_dirPathLabel.setBounds(10, y + 4, 90, rowHeight);
    m_dirPathEdit.setBounds(100, y, 400, rowHeight);
    m_browseDirBtn.setBounds(510, y, 80, rowHeight);
    y += rowHeight + padding;

    m_fileExtensionsLabel.setBounds(10, y + 4, 90, rowHeight);
    m_fileExtensionsEdit.setBounds(100, y, 400, rowHeight);
    m_fileTypeCombo.setBounds(510, y + 2, 80, rowHeight);
    y += rowHeight + padding;

    m_targetEncodingLabel.setBounds(10, y + 4, 90, rowHeight);
    m_targetEncodingCombo.setBounds(100, y, 80, rowHeight);
    y += rowHeight + padding + 6;

    m_startConversionBtn.setBounds(10, y, 90, rowHeight);
    m_createBackupCheck.setBounds(110, y, 120, rowHeight);
    y += rowHeight + padding;

    m_logList.setBounds(10, y - 4, 580, 240);
    y += 230 + padding;
    m_progressCtrl.setBounds(10, y + 2, 100, rowHeight - 4);
    m_statusText.setBounds(120, y + 5, 400, rowHeight);
    m_clearLogBtn.setBounds(510, y, 80, rowHeight);

    m_browseDirBtn.Click.connect(&MainWindow::onBrowserDirectory, this);
    m_startConversionBtn.Click.connect(&MainWindow::onStartConversion, this);
    m_fileTypeCombo.SelChange.connect(&MainWindow::onFileTypeChange, this);
    m_clearLogBtn.Click.connect(&MainWindow::onClearLog, this);
    for (const auto &encoding : { L"UTF-8", L"UTF-8-BOM", L"GBK" })
    {
        m_targetEncodingCombo.addItem(encoding);
    }
    m_targetEncodingCombo.setSelectedItem(1);  // Select UTF-8-BOM as default

    m_fileTypeCombo.addItem(L"C/C++ Files");
    m_fileTypeCombo.addItem(L"Web Files");
    m_fileTypeCombo.addItem(L"Text Files");
    m_fileTypeCombo.setSelectedItem(0);  // Select C/C++ Files as default

    // Set default values
    m_createBackupCheck.setSelected(false);
    updateStatus(L"Ready");

    setSize(vaca::Size(616, 470));
    center();
}

MainWindow::~MainWindow()
{
    m_stopConversion = true;
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }
}

void MainWindow::onBrowserDirectory(vaca::Event &ev)
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
            pFileOpen->SetTitle(L"Select a directory containing code files");

            // Show the dialog
            hr = pFileOpen->Show(getHandle());

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
                        m_dirPathEdit.setText(m_dirPath);
                        addLogMessage(std::wstring(L"Selected: ") + m_dirPath);
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

void MainWindow::onStartConversion(vaca::Event &ev)
{
    if (m_workerThread.joinable())
    {
        // If conversion is running, stop it
        stopConversion();
    }
    else
    {
        // If no conversion is running, start it
        startConversion();
    }
}

void MainWindow::stopConversion()
{
    m_stopConversion = true;
    addLogMessage(L"Stopping conversion...");
    updateStatus(L"Stopping...");
}

void MainWindow::startConversion()
{
    // Check if a conversion is already running
    if (m_workerThread.joinable())
    {
        addLogMessage(L"Conversion already in progress");
        return;
    }

    // Get directory path
    m_dirPath = m_dirPathEdit.getText();

    // Validate directory
    if (m_dirPath.empty())
    {
        vaca::MsgBox::show(this, L"Warning", L"Please select a directory first.", vaca::MsgBox::Type::Ok, vaca::MsgBox::Icon::Warning);
        return;
    }

    // Check if directory exists
    if (!fs::exists(fs::path(m_dirPath)))
    {
        vaca::MsgBox::show(this, L"Error", L"The selected directory does not exist.", vaca::MsgBox::Type::Ok, vaca::MsgBox::Icon::Error);
        return;
    }

    // Get file extensions
    m_fileExtensions = m_fileExtensionsEdit.getText();

    // Get selected encoding
    int sel = m_targetEncodingCombo.getSelectedItem();
    if (sel >= 0)
    {
        m_targetEncoding = m_targetEncodingCombo.getItemText(sel);
    }

    // Get backup option
    m_createBackup = m_createBackupCheck.isSelected();

    // Clear previous log
    while (m_logList.getItemCount() > 0)
    {
        m_logList.removeItem(0);
    }
    addLogMessage(L"Starting conversion...");

    // Change button text and disable other controls during conversion
    m_startConversionBtn.setText(L"Stop");
    m_dirPathEdit.setEnabled(false);
    m_fileExtensionsEdit.setEnabled(false);
    m_targetEncodingCombo.setEnabled(false);
    m_createBackupCheck.setEnabled(false);

    // Reset progress
    m_progressCtrl.setValue(0);
    updateStatus(L"Scanning files...");

    // Start worker thread
    m_stopConversion = false;
    m_workerThread = std::thread(&MainWindow::scanAndConvertFiles, this);
}

void MainWindow::scanAndConvertFiles()
{
    try
    {
        fs::path dirPath(m_dirPath);
        std::vector<fs::path> filesToConvert;

        // Parse file extensions
        std::vector<std::string> extensions;
        std::string extStr = wstringToString(m_fileExtensions);
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
        updateStatus(L"Scanning for files...");
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
        addLogMessage(foundMsg);

        // Convert files
        for (const auto &filepath : filesToConvert)
        {
            if (m_stopConversion)
                break;

            processedFiles++;
            updateProgress(processedFiles, totalFiles);

            std::wstring statusMsg = L"Converting: " + stringToWstring(filepath.filename().string());
            updateStatus(statusMsg);

            try
            {
                // Convert file encoding using the new convertFile function
                std::string targetEncodingStr = wstringToString(m_targetEncoding);
                ConversionInfo info = FileConverter::convertFileWithInfo(filepath, targetEncodingStr, m_createBackup);
                
                std::wstring logMsg = stringToWstring(filepath.filename().string()) + L": ";
                
                if (!info.sourceEncoding.empty())
                {
                    logMsg += stringToWstring(info.sourceEncoding) + L" -> " + stringToWstring(info.targetEncoding);
                }
                else
                {
                    logMsg += L"Unknown -> " + stringToWstring(info.targetEncoding);
                }
                
                if (info.result == ConversionResult::Success)
                {
                    logMsg += L" [OK]";
                }
                else if (info.result == ConversionResult::AlreadyTargetEncoding)
                {
                    logMsg += L" [SKIP: " + stringToWstring(conversionResultToString(info.result));
                    if (!info.errorMessage.empty())
                    {
                        logMsg += L" - " + stringToWstring(info.errorMessage);
                    }
                    logMsg += L"]";
                }
                else
                {
                    logMsg += L" [FAILED: " + stringToWstring(conversionResultToString(info.result));
                    if (!info.errorMessage.empty())
                    {
                        logMsg += L" - " + stringToWstring(info.errorMessage);
                    }
                    logMsg += L"]";
                }
                
                addLogMessage(logMsg);
            }
            catch (const std::exception &e)
            {
                std::wstring logMsg = stringToWstring(filepath.filename().string()) + L": " + stringToWstring(e.what());
                addLogMessage(logMsg);
            }
        }

        // Final update
        if (m_stopConversion)
        {
            updateStatus(L"Conversion stopped");
            addLogMessage(L"Conversion stopped");
        }
        else
        {
            updateStatus(L"Conversion completed");
            std::wstring completedMsg = L"Completed: " + std::to_wstring(processedFiles) + L" files";
            addLogMessage(completedMsg);
        }

        updateProgress(totalFiles, totalFiles);
    }
    catch (const std::exception &e)
    {
        updateStatus(L"Error during conversion");
        std::wstring errorMsg = L"Error: " + stringToWstring(e.what());
        addLogMessage(errorMsg);
    }

    // Re-enable controls
    onConversionComplete();
}

void MainWindow::onFileTypeChange(vaca::Event &ev)
{
    int sel = m_fileTypeCombo.getSelectedItem();
    if (sel >= 0)
    {
        switch (sel)
        {
        case 0:  // C/C++ Files
            setFileExtensions(L"*.h;*.hpp;*.c;*.cpp;*.cc;*.cxx");
            break;
        case 1:  // Web Files
            setFileExtensions(L"*.html;*.htm;*.css;*.js;*.php;*.asp;*.jsp");
            break;
        case 2:  // Text Files
            setFileExtensions(L"*.txt;*.md;*.ini;*.cfg;*.conf;*.xml;*.json");
            break;
        }
    }
}

void MainWindow::setFileExtensions(const std::wstring &extensions)
{
    m_fileExtensions = extensions;
    m_fileExtensionsEdit.setText(m_fileExtensions);
    addLogMessage(std::wstring(L"Extensions: ") + extensions);
}

void MainWindow::onClearLog(vaca::Event &ev)
{
    while (m_logList.getItemCount() > 0)
    {
        m_logList.removeItem(0);
    }
    addLogMessage(L"Log cleared");
}

void MainWindow::onConversionComplete()
{
    // Wait for worker thread to complete and clean up
    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }

    // Re-enable controls after conversion
    m_startConversionBtn.setText(L"Start Conversion");
    m_startConversionBtn.setEnabled(true);
    m_dirPathEdit.setEnabled(true);
    m_fileExtensionsEdit.setEnabled(true);
    m_targetEncodingCombo.setEnabled(true);
    m_createBackupCheck.setEnabled(true);
}

void MainWindow::addLogMessage(const std::wstring &message)
{
    std::lock_guard<std::mutex> lock(m_logMutex);

    // Add message to log list
    m_logList.addItem(message);

    // Scroll to bottom
    int count = m_logList.getItemCount();
    if (count > 0)
    {
        m_logList.setSelectedItem(count - 1);
    }
}

void MainWindow::updateProgress(int current, int total)
{
    if (total > 0)
    {
        int percentage = (current * 100) / total;
        m_progressCtrl.setValue(percentage);
    }
}

void MainWindow::updateStatus(const std::wstring &status)
{
    m_statusText.setText(status);
}

std::string MainWindow::wstringToString(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring MainWindow::stringToWstring(const std::string &str)
{
    if (str.empty())
        return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
