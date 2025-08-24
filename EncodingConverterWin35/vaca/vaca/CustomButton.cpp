// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2010 David Capello
//
// This file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#include "vaca/CustomButton.h"
#include "vaca/Debug.h"

using namespace vaca;

#ifndef ODS_HOTLIGHT
#  define ODS_HOTLIGHT 64
#endif

#ifndef ODS_INACTIVE
#  define ODS_INACTIVE 128
#endif

#ifndef ODS_NOACCEL
#  define ODS_NOACCEL 256
#endif

#ifndef ODS_NOFOCUSRECT
#  define ODS_NOFOCUSRECT 512
#endif

CustomButton::CustomButton(const String& text, Widget* parent, Style style)
  : Button(text, parent, style)
  , m_itemAction(0)
  , m_itemState(0)
{
}

CustomButton::~CustomButton()
{
}

bool CustomButton::onReflectedDrawItem(Graphics& g, LPDRAWITEMSTRUCT lpDrawItem)
{
  assert(lpDrawItem->CtlType == ODT_BUTTON);

  m_itemAction = lpDrawItem->itemState;
  m_itemState = lpDrawItem->itemState;

  return doPaint(g);
}

/**
   Returns true if you should draw the entire widget.
*/
bool CustomButton::isDrawEntire()
{
  return (m_itemAction & ODA_DRAWENTIRE) == ODA_DRAWENTIRE;
}

/**
   Returns true if you should draw only the focus change (check
   CustomButton::drawEntire first). Use CustomButton::isStateFocus to
   known if the Widget has or not the focus.
*/
bool CustomButton::isFocusChanged()
{
  return (m_itemAction & ODA_FOCUS) == ODA_FOCUS;
}

/**
   Returns true if you should draw only the selection state change
   (check CustomButton::drawEntire first). Use
   CustomButton::isStateSelected to known if the Widget is or not
   selected.
*/
bool CustomButton::isSelectionChanged()
{
  return (m_itemAction & ODA_SELECT) == ODA_SELECT;
}

// /// Returns true if the widget is checked. (only for CustomMenuItem)
// ///
// bool CustomButton::isStateChecked()
// {
//   return m_itemState & ODS_CHECKED;
// }

/**
   Returns true if this button is the default one.
*/
bool CustomButton::hasDefaultOptionVisualAspect()
{
  return (m_itemState & ODS_DEFAULT) == ODS_DEFAULT;
}

/**
   Returns true if this widget must be drawn disabled.
*/
bool CustomButton::hasDisabledVisualAspect()
{
  return (m_itemState & ODS_DISABLED) == ODS_DISABLED;
}

/**
   Returns true if this widget must be drawn with the focus.
*/
bool CustomButton::hasFocusVisualAspect()
{
  return (m_itemState & ODS_FOCUS) == ODS_FOCUS;
}

// /// Returns true if this menu item must be drawn grayed. (Only for CustomMenuItem)
// ///
// bool CustomButton::hasGrayedVisualAspect()
// {
//   return m_itemState & ODS_GRAYED;
// }

// bool CustomButton::hasHotLightVisualAspect()
// {
//   return m_itemState & ODS_HOTLIGHT;
// }

// bool CustomButton::hasInactiveVisualAspect()
// {
//   return m_itemState & ODS_INACTIVE;
// }

/**
   Indicates that you shouldn't draw the accelerator character
   underscored.
*/
bool CustomButton::hasNoAccelVisualAspect()
{
  return (m_itemState & ODS_NOACCEL) == ODS_NOACCEL;
}

/**
   Indicates that you shouldn't draw the focus even when the button
   has the focus (CustomButton::hasFocusVisualAspect is true).
*/
bool CustomButton::hasNoFocusRectVisualAspect()
{
  return (m_itemState & ODS_NOFOCUSRECT) == ODS_NOFOCUSRECT;
}

bool CustomButton::hasSelectedVisualAspect()
{
  return (m_itemState & ODS_SELECTED) == ODS_SELECTED;
}
