#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <filesystem>
#include "windlg/DialogMain.h"
#include "resource.h"
#include "../common/FileConverter.hpp"
#include "../common/StringUtils.hpp"

namespace fs = std::filesystem;

class EncodingDialog : public lib::DialogMain {
public:
    EncodingDialog();
    virtual ~EncodingDialog();

protected:
    // Dialog procedure
    INT_PTR dlgProc(UINT uMsg, WPARAM wp, LPARAM lp) override;

private:

    // Member variables
    std::wstring m_dirPath;
    std::wstring m_fileExtensions;
    std::wstring m_targetEncoding;
    bool m_createBackup;
    bool m_isConverting;
    bool m_stopConversion;
    std::thread m_workerThread;
    std::mutex m_logMutex;
    HBRUSH m_backgroundBrush;  // Brush for dialog background

    // Initialization methods
    void InitializeControls();
    void InitializeEncodings();
    void InitializeFileTypes();

    // Event handling methods
    void OnBrowseDir();
    void OnStartConversion();
    void OnFileTypeChanged();
    void OnClearLog();

    // Working methods
    void StartConversion();
    void StopConversion();
    void ScanAndConvertFiles();

    // Helper methods
    void AddLogMessage(const std::wstring& message);
    void UpdateProgress(int current, int total);
    void UpdateStatus(const std::wstring& status);
};
