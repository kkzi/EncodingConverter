#pragma once

#include "vaca/vaca/vaca.h"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

class MainWindow : public vaca::Frame
{
public:
    MainWindow();
    ~MainWindow();

private:
    // UI Controls
    vaca::Label m_dirPathLabel;
    vaca::TextEdit m_dirPathEdit;
    vaca::Button m_browseDirBtn;

    vaca::Label m_fileExtensionsLabel;
    vaca::TextEdit m_fileExtensionsEdit;
    vaca::ComboBox m_fileTypeCombo;

    vaca::Label m_targetEncodingLabel;
    vaca::ComboBox m_targetEncodingCombo;

    vaca::Button m_startConversionBtn;
    vaca::CheckBox m_createBackupCheck;

    vaca::ListBox m_logList;

    vaca::ProgressBar m_progressCtrl;
    vaca::Label m_statusText;
    vaca::Button m_clearLogBtn;

    // State variables
    std::wstring m_dirPath;
    std::wstring m_fileExtensions;
    std::wstring m_targetEncoding;
    bool m_createBackup;

    // Worker thread
    std::thread m_workerThread;
    std::atomic<bool> m_stopConversion;
    std::mutex m_logMutex;

    // Event handlers
    void onBrowserDirectory(vaca::Event &ev);
    void onStartConversion(vaca::Event &ev);
    void onFileTypeChange(vaca::Event &ev);
    void onClearLog(vaca::Event &ev);
    void onConversionComplete();

    // Helper methods
    void startConversion();
    void stopConversion();
    void addLogMessage(const std::wstring &message);
    void updateProgress(int current, int total);
    void updateStatus(const std::wstring &status);
    void scanAndConvertFiles();
    void setFileExtensions(const std::wstring &extensions);

    // Utility functions
    std::string wstringToString(const std::wstring &wstr);
    std::wstring stringToWstring(const std::string &str);
};