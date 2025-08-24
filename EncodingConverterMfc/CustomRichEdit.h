#pragma once

#include "stdafx.h"

class CCustomRichEdit : public CRichEditCtrl
{
public:
    CCustomRichEdit();
    virtual ~CCustomRichEdit();

protected:
    afx_msg void OnNcPaint();
    afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
    afx_msg void OnPaint();
    virtual void PreSubclassWindow();
    
    DECLARE_MESSAGE_MAP()
};
