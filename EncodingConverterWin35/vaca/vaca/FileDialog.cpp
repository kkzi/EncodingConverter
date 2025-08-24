// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2010 David Capello
//
// This file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#include "vaca/FileDialog.h"
#include "vaca/Widget.h"
#include "vaca/Application.h"
#include "vaca/System.h"
#include "vaca/String.h"

// 32k is the limit for Win95/98/Me/NT4/2000/XP with ANSI version
#define FILENAME_BUFSIZE (1024*32)

using namespace vaca;

// ======================================================================
// FileDialog

FileDialog::FileDialog(const String& title, Widget* parent)
  : CommonDialog(parent)
  , m_title(title)
  , m_defaultExtension(L"")
  , m_showReadOnly(false)
  , m_showHelp(false)
  , m_defaultFilter(0)
{
  m_fileName = new Char[FILENAME_BUFSIZE];
  ZeroMemory(m_fileName, sizeof(Char[FILENAME_BUFSIZE]));
}

FileDialog::~FileDialog()
{
  delete[] m_fileName;
}

/**
   Sets the title text.
*/
void FileDialog::setTitle(const String& str)
{
  m_title = str;
}

/**
   Sets the default extension to add to the entered file name when an
   extension isn't specified by the user. By default it's an empty
   string.
*/
void FileDialog::setDefaultExtension(const String& str)
{
  m_defaultExtension = str;
}

/**
   Sets the property that indicates if the dialog should show the read
   only check box. By default it's false: the button is hidden.
*/
void FileDialog::setShowReadOnly(bool state)
{
  m_showReadOnly = state;
}

/**
   Sets the property that indicates if the dialog should show the help
   button. By default it's false: the button is hidden.
*/
void FileDialog::setShowHelp(bool state)
{
  m_showHelp = state;
}

void FileDialog::addFilter(const String& extensions, const String& description, bool defaultFilter)
{
  m_filters.push_back(std::make_pair(extensions, description));

  if (defaultFilter)
    m_defaultFilter = m_filters.size();
}

String FileDialog::getFileName()
{
  return String(m_fileName);
}

void FileDialog::setFileName(const String& fileName)
{
  copy_string_to(fileName, m_fileName, FILENAME_BUFSIZE);
}

LPTSTR FileDialog::getOriginalFileName()
{
  return m_fileName;
}

bool FileDialog::doModal()
{
  // make the m_filtersString
  m_filtersString.clear();
  for (std::vector<std::pair<String, String> >::iterator it=m_filters.begin();
       it!=m_filters.end(); ++it) {
    m_filtersString.append(it->first);
    m_filtersString.push_back('\0');
    m_filtersString.append(it->second);
    m_filtersString.push_back('\0');
  }
  m_filtersString.push_back('\0');

  // fill the OPENFILENAME structure
  OPENFILENAME ofn;

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = getParentHandle();
  ofn.hInstance = Application::getHandle();
  ofn.lpstrFilter = m_filtersString.c_str();
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter = 0;
  ofn.nFilterIndex = m_defaultFilter;
  ofn.lpstrFile = m_fileName;
  ofn.nMaxFile = FILENAME_BUFSIZE;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = m_title.c_str();
  ofn.Flags = 0
// #define OFN_CREATEPROMPT 0x2000
// #define OFN_ENABLEHOOK 32
    | OFN_ENABLESIZING
// #define OFN_ENABLETEMPLATE 64
// #define OFN_ENABLETEMPLATEHANDLE 128
    | OFN_EXPLORER
// #define OFN_EXTENSIONDIFFERENT 0x400
    | (m_showReadOnly ? 0: OFN_HIDEREADONLY)
    | OFN_LONGNAMES
    | OFN_NOCHANGEDIR
// | OFN_NODEREFERENCELINKS
// | OFN_NOLONGNAMES
// | OFN_NONETWORKBUTTON
// | OFN_NOREADONLYRETURN
// | OFN_NOTESTFILECREATE
// | OFN_NOVALIDATE
// #define OFN_READONLY 1
// #define OFN_SHAREAWARE 0x4000
    | (m_showHelp ? OFN_SHOWHELP: 0)
    ;
// | OFN_SHAREFALLTHROUGH
// | OFN_SHARENOWARN
// | OFN_SHAREWARN
// | OFN_NODEREFERENCELINKS
// #if (_WIN32_WINNT >= 0x0500)
// | OFN_DONTADDTORECENT
// #endif

  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = m_defaultExtension.c_str();
  ofn.lCustData = 0;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;
#if (_WIN32_WINNT >= 0x0500)
  ofn.pvReserved = NULL;
  ofn.dwReserved = 0;
  ofn.FlagsEx = 0;
#endif

  return showDialog(&ofn);
}

// ======================================================================
// OpenFileDialog

OpenFileDialog::OpenFileDialog(const String& title, Widget* parent)
  : FileDialog(title, parent)
  , m_multiselect(false)
{
}

OpenFileDialog::~OpenFileDialog()
{
}

/**
   By default it's false.
*/
void OpenFileDialog::setMultiselect(bool state)
{
  m_multiselect = state;
}

std::vector<String> OpenFileDialog::getFileNames()
{
  std::vector<String> result;

  if (m_multiselect) {
    LPTSTR ptr, start;
    String path;

    for (ptr=start=getOriginalFileName(); ; ++ptr) {
      if (*ptr == '\0') {
	if (path.empty())
	  path = start;
	else
	  result.push_back(path + L"\\" + start);

	if (*(++ptr) == '\0')
	  break;
	start = ptr;
      }
    }
  }

  // empty results? one file-name selected
  if (result.empty())
    result.push_back(getFileName());

  return result;
}

bool OpenFileDialog::showDialog(LPOPENFILENAME lpofn)
{
  lpofn->Flags |= 0
    | (m_multiselect ? OFN_ALLOWMULTISELECT: 0)
    | OFN_FILEMUSTEXIST
    | OFN_PATHMUSTEXIST
    ;

  return GetOpenFileName(lpofn) != FALSE;
}

// ======================================================================
// SaveFileDialog

SaveFileDialog::SaveFileDialog(const String& title, Widget* parent)
  : FileDialog(title, parent)
{
}

SaveFileDialog::~SaveFileDialog()
{
}

bool SaveFileDialog::showDialog(LPOPENFILENAME lpofn)
{
  lpofn->Flags |= 0
    | OFN_PATHMUSTEXIST
    | OFN_OVERWRITEPROMPT
    ;

  return GetSaveFileName(lpofn) != FALSE;
}
