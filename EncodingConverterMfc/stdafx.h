#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#include <targetver.h>

#define _AFX_ALL_WARNINGS

// Enable visual styles
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#include <afxdialogex.h>
#include <afxcmn.h>
#include <commctrl.h>
#include <uxtheme.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>

namespace fs = std::filesystem;

// UNICODE helper functions
inline CString StringToCString(const std::string& str)
{
    return CString(CA2T(str.c_str()));
}

inline std::string CStringToString(const CString& cstr)
{
    return std::string(CT2A(cstr.GetString()));
}
