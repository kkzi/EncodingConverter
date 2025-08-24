#pragma once

#include "resource.h"
#include "stdafx.h"
#include "CustomRichEdit.h"
#include <vector>
#include <string>
#include <filesystem>

class CMainDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CMainDialog)

public:
    CMainDialog(CWnd *pParent = nullptr);
    virtual ~CMainDialog();

protected:
    virtual void DoDataExchange(CDataExchange *pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedBrowseDir();
    afx_msg void OnBnClickedStartConversion();
    afx_msg void OnCbnSelchangeFileTypeCombo();
    afx_msg void OnBnClickedClearLog();
    afx_msg void OnBnClickedStopConversion();
    afx_msg LRESULT OnConversionComplete(WPARAM wParam, LPARAM lParam);
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);

    DECLARE_MESSAGE_MAP()

private:
    // UI Controls
    CEdit m_dirPathEdit;
    CEdit m_fileExtensionsEdit;
    CComboBox m_targetEncodingCombo;
    CComboBox m_fileTypeCombo;
    CButton m_createBackupCheck;
    CButton m_startConversionBtn;
    CProgressCtrl m_progressCtrl;
    CStatic m_statusText;
    CButton m_clearLogBtn;
    CCustomRichEdit m_logList;

    // State variables
    CString m_dirPath;
    CString m_fileExtensions;
    CString m_targetEncoding;
    BOOL m_createBackup;

    // Worker thread
    std::thread m_workerThread;
    std::atomic<bool> m_stopConversion;
    std::mutex m_logMutex;

    // Helper methods
    void InitializeEncodings();
    void InitializeFileTypes();
    void StartConversion();
    void StopConversion();
    void AddLogMessage(const CString &message, COLORREF textColor = RGB(0, 0, 0));
    void PostLogMessage(const CString &message);  // Thread-safe log posting
    void UpdateProgress(int current, int total);
    void UpdateStatus(const CString &status);
    void ScanAndConvertFiles();
    void SetFileExtensions(const CString &extensions);
};
