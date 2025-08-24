// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2010 David Capello
//
// This file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#if 0

#include "vaca/BasicDockArea.h"
#include "vaca/DockBar.h"
#include "vaca/Frame.h"
#include "vaca/Point.h"
#include "vaca/Debug.h"
#include "vaca/Pen.h"
#include "vaca/PreferredSizeEvent.h"

using namespace vaca;

/**
   Information about the docked position of the DockBar in a BasicDockArea.
*/
class BasicDockInfo : public DockInfo
{
public:

  Side side;       // BasicDockInfo doesn't save a DockArea pointer, just the side of the DockArea
  Rect bounds;     // Bounds inside the BasicDockAreaa.

  virtual Size getSize() { return bounds.getSize(); }
  virtual Side getSide() { return side; }
};

BasicDockArea::BasicDockArea(Side side, Widget* parent, Style style)
  : DockArea(side, parent, style)
{
}

BasicDockArea::~BasicDockArea()
{
}

bool BasicDockArea::hitTest(DockBar* bar, const Point& cursor, const Point& anchor, bool fromInside)
{
  Rect bounds = getAbsoluteClientBounds();
  Size sz;
  Rect rc;

  if (fromInside)
    sz = bounds.getSize().createUnion(bar->getDockedSize(getSide()));
  else
    sz = bounds.getSize().createUnion(Size(32, 32));

  switch (getSide()) {
    case Side::Left:   rc = Rect(bounds.x, bounds.y, sz.w, bounds.h); break;
    case Side::Top:    rc = Rect(bounds.x, bounds.y, bounds.w, sz.h); break;
    case Side::Right:  rc = Rect(bounds.x+bounds.w-sz.w, bounds.y, sz.w, bounds.h); break;
    case Side::Bottom: rc = Rect(bounds.x, bounds.y+bounds.h-sz.h, bounds.w, sz.h); break;
  }

  return rc.contains(cursor);
}

DockInfo* BasicDockArea::createDefaultDockInfo(DockBar* bar)
{
  BasicDockInfo* dockInfo = new BasicDockInfo();
  dockInfo->side = getSide();
  dockInfo->bounds = Rect(Point(0, 0), bar->getDockedSize(getSide()));
  return dockInfo;
}

DockInfo* BasicDockArea::createDockInfo(DockBar* bar, const Point& cursor, const Point& anchor)
{
  Rect bounds = getAbsoluteClientBounds();
  Size size = bar->getDockedSize(getSide());
  Point origin(0, 0);

  if (isHorizontal()) {
    origin.x = (cursor.x - bounds.x) - anchor.x;

    if (origin.x < 0)
      origin.x = 0;
    else if (origin.x+size.w > bounds.w)
      origin.x = bounds.w-size.w;
  }
  else {
    origin.y = (cursor.y - bounds.y) - anchor.y;

    if (origin.y < 0)
      origin.y = 0;
    else if (origin.y+size.h > bounds.h)
      origin.y = bounds.h-size.h;
  }

  BasicDockInfo* dockInfo = new BasicDockInfo();
  dockInfo->side = getSide();
  dockInfo->bounds = Rect(origin, size);
  return dockInfo;
}

void BasicDockArea::drawXorTracker(Graphics& g, DockInfo* _dockInfo)
{
  BasicDockInfo* dockInfo = static_cast<BasicDockInfo*>(_dockInfo);
  Rect bounds = getAbsoluteClientBounds();
  Rect dockBarBounds = dockInfo->bounds;
  Rect externRect;
  Rect internRect;
  Size sz = bounds.getSize().createUnion(dockBarBounds.getSize());

  switch (getSide()) {

    case Side::Left:
      externRect = Rect(bounds.x, bounds.y, sz.w, bounds.h);
      break;

    case Side::Top:
      externRect = Rect(bounds.x, bounds.y, bounds.w, sz.h);
      break;

    case Side::Right:
      externRect = Rect(bounds.x+bounds.w-sz.w, bounds.y, sz.w, bounds.h);
      break;

    case Side::Bottom:
      externRect = Rect(bounds.x, bounds.y+bounds.h-sz.h, bounds.w, sz.h);
      break;

  }

  internRect = Rect(externRect.getOrigin() + dockBarBounds.getOrigin(),
		    dockBarBounds.getSize());

  g.setRop2(R2_NOTXORPEN);
  Pen pen(Color::Black);	// TODO it's necessary... maybe XorPen?
//   g.drawRect(pen, externRect);
//   g.drawRect(pen, internRect.shrink(1));
  g.drawRect(pen, internRect);
  g.setRop2(R2_COPYPEN);
}

void BasicDockArea::layout()
{
  WidgetList children = getChildren();
  WidgetList::iterator it;

  for (it=children.begin(); it!=children.end(); ++it) {
    DockBar* dockBar = static_cast<DockBar*>(*it);
    BasicDockInfo* dockInfo = static_cast<BasicDockInfo*>(dockBar->getDockInfo());

    assert(dockInfo != NULL);

    dockBar->setBounds(dockInfo->bounds);
    dockBar->layout();
  }
}

void BasicDockArea::onPreferredSize(PreferredSizeEvent& ev)
{
  WidgetList children = getChildren();
  WidgetList::iterator it;
  Size sz;

  for (it=children.begin(); it!=children.end(); ++it) {
    DockBar* dockBar = static_cast<DockBar*>(*it);
    DockInfo* dockInfo = dockBar->getDockInfo();

    assert(dockInfo != NULL);

    if (isHorizontal())
      sz.h = sz.h < dockInfo->getSize().h ? dockInfo->getSize().h: sz.h;
    else
      sz.w = sz.w < dockInfo->getSize().w ? dockInfo->getSize().w: sz.w;
  }

  ev.setPreferredSize(sz);
}

#endif
