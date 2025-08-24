#include "MainDialog.h"
#include "stdafx.h"

class CEncodingConverterApp : public CWinApp
{
public:
    CEncodingConverterApp();

public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CEncodingConverterApp, CWinApp)
END_MESSAGE_MAP()

CEncodingConverterApp::CEncodingConverterApp()
{
}

BOOL CEncodingConverterApp::InitInstance()
{
    CWinApp::InitInstance();

    // Initialize Rich Edit control
    if (!AfxInitRichEdit2())
    {
        AfxMessageBox(_T("Failed to initialize Rich Edit control"));
        return FALSE;
    }

    CMainDialog dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return TRUE;
}

CEncodingConverterApp theApp;
