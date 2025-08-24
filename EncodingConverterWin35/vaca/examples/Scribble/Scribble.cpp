// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
//
// This file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#include <vaca/vaca.h>
#include "../resource.h"

using namespace vaca;

class Scribble : public Widget
{
  Image m_image;
  Point m_point[3];
  bool m_erasing;

public:

  Scribble(Widget* parent)
    : Widget(parent)
  {
    setBgColor(Color::White);
  }

protected:

  virtual void onResize(ResizeEvent& ev)
  {
    Size imageSize = getClientBounds().getSize();
    if (imageSize.w > 0 && imageSize.h > 0) {
      Image newImage(imageSize);

      Graphics& g = newImage.getGraphics();

      // clear the new image with a white background
      Brush whiteBrush(Color::White);
      g.fillRect(whiteBrush, 0, 0, imageSize.w, imageSize.h);

      // draw the old image in the center of the new image
      if (m_image.isValid())
	g.drawImage(m_image, Point(imageSize/2 - m_image.getSize()/2));

      // Assign the new image to the member variable m_image. Remember
      // that Image class is a shared-pointer, so here the old image
      // referenced by m_image is automatically deleted.  This is the
      // magic of SharedPtrs!
      m_image = newImage;

      // redraw all the image
      invalidate(false);
    }
    Widget::onResize(ev);
  }

  virtual void onPaint(PaintEvent& ev)
  {
    Graphics& g = ev.getGraphics();

    // draw only the clip area
    Rect rc = g.getClipBounds();
    g.drawImage(m_image, rc.getOrigin(), rc);
  }

  virtual void onMouseDown(MouseEvent& ev)
  {
    if (!hasCapture()) {
      captureMouse();

      for (int c=0; c<3; ++c)
	m_point[c] = ev.getPoint();

      // with right mouse button we erase
      m_erasing = ev.getButton() == MouseButton::Right;
    }
  }

  virtual void onMouseMove(MouseEvent& ev)
  {
    if (hasCapture()) {
      // get the graphics from the to draw into the image
      Graphics& g = m_image.getGraphics();

      // rotate points
      m_point[0] = m_point[1];
      m_point[1] = m_point[2];
      m_point[2] = ev.getPoint();

      // pen
      Color color(m_erasing ? Color::White: Color::Black);
      Pen pen(color, m_erasing ? 64: 1);

      // draw (or erase)
      g.drawLine(pen, m_point[1], m_point[2]);

      // we are drawing?
      if (!m_erasing) {
	// this adds an extra style to the trace (more realistic)
	GraphicsPath path;
	path.moveTo(m_point[0]);
	path.lineTo(m_point[1]);
	path.lineTo(m_point[2]);

	Brush brush(color);
	g.fillPath(path, brush, Point(0, 0));
      }

      // invalidate the drawn area
      Point minPoint = m_point[0];
      Point maxPoint = m_point[0];
      for (int c = 1; c < 3; ++c) {
	if (minPoint.x > m_point[c].x) minPoint.x = m_point[c].x;
	if (minPoint.y > m_point[c].y) minPoint.y = m_point[c].y;
	if (maxPoint.x < m_point[c].x) maxPoint.x = m_point[c].x;
	if (maxPoint.y < m_point[c].y) maxPoint.y = m_point[c].y;
      }
      invalidate(Rect(minPoint-pen.getWidth(),
		      maxPoint+pen.getWidth()), false);
    }
  }

  virtual void onMouseUp(MouseEvent& ev)
  {
    if (hasCapture()) {
      // we can release the capture (remember to check hasCapture()
      // before to release the mouse capture)
      releaseMouse();
    }
  }

};

class MainFrame : public Frame
{
  Scribble m_scrible;

public:

  MainFrame()
    : Frame(L"Scribble")
    , m_scrible(this)
  {
    setLayout(new ClientLayout);
  }

};

//////////////////////////////////////////////////////////////////////

int VACA_MAIN()
{
  Application app;
  MainFrame frm;
  frm.setIcon(ResourceId(IDI_VACA));
  frm.setVisible(true);
  app.run();
  return 0;
}
