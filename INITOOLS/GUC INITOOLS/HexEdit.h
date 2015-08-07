#pragma once


// CHexEdit

class CHexEdit : public CEdit
{
	DECLARE_DYNAMIC(CHexEdit)

public:
	CHexEdit();
	virtual ~CHexEdit();

//protected:
	DECLARE_MESSAGE_MAP()

public:
//	unsigned char buffer[32 * 1024];	//32KB
	DWORD buffer_len;	// max 32 * 1024
	unsigned char map[8 * 1024][75];	//2 * 1024 rows,16byte per row in buffer, other is stuf
	int map_row;

	int m_nullWidth;
	int trows;	//display rows
	int nrow;   // index of first display rows in map
	int frows;
	bool lb_down; //state of left mouse button down;

	CPoint pt_c;
	CPoint pt_begin;
	CPoint pt_stop;

	int RA_begin;
	int CA_begin;
	int RA_stop;
	int CA_stop;
	bool selected;

	CRect rect;
	CFont font;
	CPen pen_line;

	void SetContent(void* data, long len);
	void GetContent(void* data, DWORD len);
	void PosCal(CPoint* point);
	void SelCal();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


