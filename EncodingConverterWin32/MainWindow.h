#pragma once

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

namespace fs = std::filesystem;

// DPI awareness support
#include <shellscalingapi.h>
#pragma comment(lib, "shcore.lib")

// Control IDs
#define IDC_DIR_PATH                    1001
#define IDC_BROWSE_DIR                  1002
#define IDC_FILE_EXTENSIONS             1003
#define IDC_TARGET_ENCODING             1004
#define IDC_CREATE_BACKUP               1005
#define IDC_START_CONVERSION            1006
#define IDC_PROGRESS                    1007
#define IDC_STATUS                      1008
#define IDC_CLEAR_LOG                   1009
#define IDC_LOG_LIST                    1010
#define IDC_FILE_TYPE_COMBO             1011


// Custom messages
#define WM_CONVERSION_COMPLETE          (WM_USER + 1)
#define WM_UPDATE_PROGRESS              (WM_USER + 2)
#define WM_UPDATE_STATUS                (WM_USER + 3)
#define WM_ADD_LOG                      (WM_USER + 4)

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInstance);
    void Show(int nCmdShow);
    
    // DPI support methods
    void InitializeDPI();
    float GetDPIScale() const;
    int ScaleForDPI(int value) const;
    void UpdateDpiDependentResources();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Message handlers
    void OnCreate();
    void OnSize(int width, int height);
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnBrowseDir();
    void OnStartConversion();
    void OnFileTypeChange();
    void OnClearLog();
    void OnConversionComplete();
    void OnUpdateProgress(WPARAM wParam, LPARAM lParam);
    void OnUpdateStatus(LPARAM lParam);
    void OnAddLog(LPARAM lParam);
    void OnDpiChanged(WPARAM wParam, LPARAM lParam);

    // Helper methods
    void InitializeControls();
    void InitializeEncodings();
    void InitializeFileTypes();
    void BrowseForDirectory();
    void StartConversion();
    void StopConversion();
    void ScanAndConvertFiles();
    void SetFileExtensions(const std::wstring& extensions);
    void AddLogMessage(const std::wstring& message);
    void UpdateStatus(const std::wstring& status);
    void UpdateProgress(int current, int total);
    // Utility functions (moved to StringUtils)
    // Simple encoding detection and conversion (placeholder)
    std::string DetectFileEncoding(const fs::path& filepath);
    bool ConvertFileEncoding(const fs::path& filepath, const std::string& targetEncoding);

private:
    HINSTANCE m_hInstance;
    HWND m_hwnd;
    HFONT m_hFont;
    HBRUSH m_hBackgroundBrush;
    HBRUSH m_hControlBrush;
    
    // DPI support
    float m_dpiScale;
    UINT m_dpi;
    
    // Control handles
    HWND m_hDirPathEdit;
    HWND m_hBrowseDirBtn;
    HWND m_hFileExtensionsEdit;
    HWND m_hTargetEncodingCombo;
    HWND m_hFileTypeCombo;
    HWND m_hCreateBackupCheck;
    HWND m_hStartConversionBtn;
    HWND m_hProgressCtrl;
    HWND m_hStatusText;
    HWND m_hClearLogBtn;
    HWND m_hLogList;

    // State variables
    std::wstring m_dirPath;
    std::wstring m_fileExtensions;
    std::wstring m_targetEncoding;
    bool m_createBackup;
    
    // Worker thread
    std::thread m_workerThread;
    std::atomic<bool> m_stopConversion;
    std::mutex m_logMutex;
};