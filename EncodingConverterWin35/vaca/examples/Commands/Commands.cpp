// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
//
// This file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#include <vaca/vaca.h>
#include "resource.h"

using namespace vaca;

#include "../Console.h"

//////////////////////////////////////////////////////////////////////
// MainFrame class

class MainFrame : public Frame
{
  Console m_editor;
  Console m_console;
  //ToolBar m_toolBar;
  ImageList m_imageList;

public:
  MainFrame();
  Console* getEditor();
  Console* getConsole();
  void showToolBar();
  bool isToolBarVisible();
};


//////////////////////////////////////////////////////////////////////
// Example class

class Example : public Application
	      , public CommandsClient // it's a container of commands
{
  MainFrame m_mainFrame;

public:
  Example();

  Console* getEditor();
  Console* getConsole();

private:
  virtual void main();
};

#define TheEditor							\
  (static_cast<Example*>(Application::getInstance())->getEditor())

#define TheConsole							\
  (static_cast<Example*>(Application::getInstance())->getConsole())

//////////////////////////////////////////////////////////////////////
// AppCommand - a command of the application

class AppCommand : public Command
{
  String m_name;
public:
  AppCommand(CommandId id, const String& n)
    : Command(id)
    , m_name(n) {
  }
  void execute() {
    TheConsole->println(m_name + L"::execute");
  }
};

//////////////////////////////////////////////////////////////////////
// Specific commands

class FileNewCommand : public AppCommand
{
public:
  FileNewCommand() : AppCommand(ID_FILE_NEW, L"FileNew") { }
  virtual void execute() {
    AppCommand::execute();
  }
};

class FileOpenCommand : public AppCommand
{
public:
  FileOpenCommand() : AppCommand(ID_FILE_OPEN, L"FileOpen") { }
  virtual void execute() {
    AppCommand::execute();
  }
};

class FileSaveCommand : public AppCommand
{
public:
  FileSaveCommand() : AppCommand(ID_FILE_SAVE, L"FileSave") { }
  virtual void execute() {
    AppCommand::execute();
  }
};

class FileCloseCommand : public AppCommand
{
public:
  FileCloseCommand() : AppCommand(ID_FILE_CLOSE, L"FileClose") { }
  virtual void execute() {
    AppCommand::execute();
  }
};

class FileExitCommand : public AppCommand
{
public:
  FileExitCommand() : AppCommand(ID_FILE_EXIT, L"FileExit") { }
  virtual void execute() {
    AppCommand::execute();
    CurrentThread::breakMessageLoop();
  }
};

class EditCutCommand : public AppCommand
{
public:
  EditCutCommand() : AppCommand(ID_EDIT_CUT, L"EditCut") { }
  virtual void execute() {
    AppCommand::execute();
    TheEditor->cut();
  }
  virtual bool isEnabled() {
    int start, end;
    TheEditor->getSelection(start, end);
    return start != end;
  }
};

class EditCopyCommand : public AppCommand
{
public:
  EditCopyCommand() : AppCommand(ID_EDIT_COPY, L"EditCopy") { }
  virtual void execute() {
    AppCommand::execute();
    TheEditor->copy();
  }
  virtual bool isEnabled() {
    int start, end;
    TheEditor->getSelection(start, end);
    return start != end;
  }
};

class EditPasteCommand : public AppCommand
{
public:
  EditPasteCommand() : AppCommand(ID_EDIT_PASTE, L"EditPaste") { }
  virtual void execute() {
    AppCommand::execute();
    TheEditor->paste();
  }
  virtual bool isEnabled() {
    return Clipboard().isText();
  }
};

class ViewToolBarCommand : public AppCommand
{
  MainFrame* m_frame;
public:
  ViewToolBarCommand(MainFrame* frame)
    : AppCommand(ID_VIEW_TOOLBAR, L"ViewToolBar")
    , m_frame(frame) { }
  virtual void execute() {
    AppCommand::execute();
    m_frame->showToolBar();
  }
  virtual bool isEnabled() {
    return !m_frame->isToolBarVisible();
  }
};

//////////////////////////////////////////////////////////////////////
// MainFrame implementation

MainFrame::MainFrame()
  : Frame(L"Commands")
  , m_editor(this)
  , m_console(this)
    //, m_toolBar(L"Standard", this, ToolSet::Styles::Default +
    //			 ToolSet::Styles::Flat)
  , m_imageList(ResourceId(IDB_TOOLBAR), 16, Color(236, 233, 216))
{
  setIcon(ResourceId(IDI_VACA));
  setLayout(new BoxLayout(Orientation::Vertical, true));
  setMenuBar(new MenuBar);

  m_editor.setText(L"This is the current document");

  m_console.setBgColor(Color::Black);
  m_console.setFgColor(Color(0, 220, 0));

  // add menus
  Menu* fileMenu = new Menu(L"&File");
  Menu* editMenu = new Menu(L"&Edit");

  MenuItem* newMenuItem     = new MenuItem(L"&New", ID_FILE_NEW);
  MenuItem* openMenuItem    = new MenuItem(L"&Open", ID_FILE_OPEN);
  MenuItem* saveMenuItem    = new MenuItem(L"&Save", ID_FILE_SAVE);
  MenuItem* closeMenuItem   = new MenuItem(L"&Close", ID_FILE_CLOSE);
  MenuItem* exitMenuItem    = new MenuItem(L"E&xit", ID_FILE_EXIT);
  MenuItem* cutMenuItem     = new MenuItem(L"Cu&t", ID_EDIT_CUT);
  MenuItem* copyMenuItem    = new MenuItem(L"&Copy", ID_EDIT_COPY);
  MenuItem* pasteMenuItem   = new MenuItem(L"&Paste", ID_EDIT_PASTE);
  MenuItem* toolBarMenuItem = new MenuItem(L"&View Tool Bar", ID_VIEW_TOOLBAR);

  fileMenu->add(newMenuItem);
  fileMenu->add(openMenuItem);
  fileMenu->addSeparator();
  fileMenu->add(saveMenuItem);
  fileMenu->add(closeMenuItem);
  fileMenu->addSeparator();
  fileMenu->add(exitMenuItem);
  editMenu->add(cutMenuItem);
  editMenu->add(copyMenuItem);
  editMenu->add(pasteMenuItem);

  getMenuBar()->add(fileMenu);
  getMenuBar()->add(editMenu);
  getMenuBar()->add(toolBarMenuItem);

  // // setup the tool bar
  // m_toolBar.setImageList(m_imageList);
  // m_toolBar.addButton(new ToolButton(ID_FILE_NEW, 0));
  // m_toolBar.addButton(new ToolButton(ID_FILE_OPEN, 1));
  // m_toolBar.addButton(new ToolButton(ID_FILE_SAVE, 2));
  // m_toolBar.addSeparator();
  // m_toolBar.addButton(new ToolButton(ID_EDIT_CUT, 3));
  // m_toolBar.addButton(new ToolButton(ID_EDIT_COPY, 4));
  // m_toolBar.addButton(new ToolButton(ID_EDIT_PASTE, 5));

  // setup the defaults dock areas
  // defaultDockAreas();

  // put the tool bar in the top dock area
  // m_toolBar.dockIn(getDockArea(Side::Top));
}

Console* MainFrame::getEditor()
{
  return &m_editor;
}

Console* MainFrame::getConsole()
{
  return &m_console;
}

void MainFrame::showToolBar()
{
  // m_toolBar.setVisible(true);
}

bool MainFrame::isToolBarVisible()
{
  // return m_toolBar.isVisible();
  return true;
}

//////////////////////////////////////////////////////////////////////
// Example implementation

Example::Example()
{
  addCommand(new FileNewCommand);
  addCommand(new FileOpenCommand);
  addCommand(new FileSaveCommand);
  addCommand(new FileCloseCommand);
  addCommand(new FileExitCommand);
  addCommand(new EditCutCommand);
  addCommand(new EditCopyCommand);
  addCommand(new EditPasteCommand);
  addCommand(new ViewToolBarCommand(&m_mainFrame));
}

Console* Example::getEditor()
{
  return m_mainFrame.getEditor();
}

Console* Example::getConsole()
{
  return m_mainFrame.getConsole();
}

void Example::main()
{
  m_mainFrame.setVisible(true);
}

//////////////////////////////////////////////////////////////////////
// WinMain

int VACA_MAIN()
{
  Example app;
  app.run();
  return 0;
}
