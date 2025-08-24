// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2010 David Capello
//
// This file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>

#include "vaca/Color.h"
#include "vaca/GdiObject.h"
#include "vaca/win32.h"

class vaca::Brush::BrushImpl : public GdiObject<HBRUSH>
{
public:

  BrushImpl()
    : GdiObject<HBRUSH>(CreateSolidBrush(RGB(0, 0, 0))) {
  }

  BrushImpl(const Color& color)
    : GdiObject<HBRUSH>(CreateSolidBrush(convert_to<COLORREF>(color))) {
  }

  ~BrushImpl() {
  }

  Color getColor() const {
    LOGBRUSH lb;
    assert(getHandle());
    ::GetObject(getHandle(), sizeof(LOGBRUSH), &lb);
    return convert_to<Color>(lb.lbColor);
  }

};

template<> HBRUSH vaca::convert_to(const Brush& brush)
{
  // 1) getHandle is defined in GdiObject<HBRUSH> class
  // 2) convert_to<> is friend of Brush class
  return brush.m_impl->getHandle();
}
