#include "CustomRichEdit.h"

BEGIN_MESSAGE_MAP(CCustomRichEdit, CRichEditCtrl)
    ON_WM_NCPAINT()
    ON_WM_NCCALCSIZE()
    ON_WM_PAINT()
END_MESSAGE_MAP()

CCustomRichEdit::CCustomRichEdit()
{
}

CCustomRichEdit::~CCustomRichEdit()
{
}

void CCustomRichEdit::OnNcPaint()
{
    // Call default first
    Default();
    
    // Get window DC for non-client area painting
    CWindowDC dc(this);
    
    // Get window rect
    CRect rect;
    GetWindowRect(&rect);
    rect.OffsetRect(-rect.left, -rect.top);
    
    // Create pen for 1px border
    CPen pen(PS_SOLID, 1, RGB(128, 128, 128)); // Gray border
    CPen* pOldPen = dc.SelectObject(&pen);
    
    // Create brush for no fill
    CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
    
    // Draw 1px border rectangle
    dc.Rectangle(&rect);
    
    // Restore old pen and brush
    dc.SelectObject(pOldPen);
    dc.SelectObject(pOldBrush);
}

void CCustomRichEdit::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
    // Call default first
    CRichEditCtrl::OnNcCalcSize(bCalcValidRects, lpncsp);
    
    // Adjust client area to account for our custom border
    if (bCalcValidRects && lpncsp)
    {
        lpncsp->rgrc[0].left += 1;
        lpncsp->rgrc[0].top += 1;
        lpncsp->rgrc[0].right -= 1;
        lpncsp->rgrc[0].bottom -= 1;
    }
}

void CCustomRichEdit::OnPaint()
{
    // Let the base class paint first
    CRichEditCtrl::OnPaint();
}

void CCustomRichEdit::PreSubclassWindow()
{
    CRichEditCtrl::PreSubclassWindow();
    
    // Remove existing border styles
    ModifyStyle(WS_BORDER, 0);
    ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0);
    
    // Force initial border redraw
    SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}
