// HexEdit.cpp : 实现文件
//

#include "stdafx.h"
#include "HexEdit.h"


// CHexEdit
unsigned char hextable[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

IMPLEMENT_DYNAMIC(CHexEdit, CEdit)
CHexEdit::CHexEdit()
{
	lb_down = false;
	selected = false;
	buffer_len=0;
	map_row=0;
	nrow=0;
	trows=0;
	pt_c.x = 80;
	pt_c.y = 20;
	pen_line.CreatePen(PS_SOLID, 1, RGB(162,0,70));
	VERIFY(font.CreateFont(			14,                        // nHeight
									0,                         // nWidth
									0,                         // nEscapement
									0,                         // nOrientation
									FW_NORMAL,                 // nWeight
									FALSE,                     // bItalic
									FALSE,                     // bUnderline
									0,                         // cStrikeOut
									ANSI_CHARSET,              // nCharSet
									OUT_DEFAULT_PRECIS,        // nOutPrecision
									CLIP_DEFAULT_PRECIS,       // nClipPrecision
									DEFAULT_QUALITY,           // nQuality
									DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
									"Arial"));                 // lpszFacename

}

CHexEdit::~CHexEdit()
{
	pen_line.DeleteObject();
	font.DeleteObject();
}


BEGIN_MESSAGE_MAP(CHexEdit, CEdit)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()



// CHexEdit 消息处理程序

void CHexEdit::OnPaint()
{
	CPaintDC pdc(this); // device context for painting
	GetClientRect(&rect);
	
	CDC	dc;
	dc.CreateCompatibleDC(CDC::FromHandle(pdc.m_ps.hdc));
	CBitmap bm;
	bm.CreateCompatibleBitmap(CDC::FromHandle(pdc.m_ps.hdc), rect.Width(), rect.Height());
	dc.SelectObject(bm);
	dc.SetBoundsRect(&rect,DCB_DISABLE);								
	dc.SelectObject(&font);
	dc.GetCharWidth('A', 'A', &m_nullWidth);

	trows=rect.Height()%(m_nullWidth*2);
	if(trows==0)
		trows=rect.Height()/(m_nullWidth*2);
	else
		trows=rect.Height()/(m_nullWidth*2)+1;

	trows -= 2;
	CString temp;
	int i;
	int x=0;
	int y=0;

	////////////////////////////////////////////////draw selected area,should before draw map
	if(selected)
	{
		CRect rt1;
		CPoint begin,stop,index;
		int temp_int;

		CPen pen_sl;
		pen_sl.CreatePen(PS_SOLID, 1, RGB(122,0,0));
		dc.SelectObject(&pen_sl);

		begin.x = CA_begin;
		begin.y = RA_begin;
		stop.x = CA_stop;
		stop.y = RA_stop;

		if(pt_begin.y > pt_stop.y)
			stop.x++;

		if(begin.y < nrow)	
		{
			begin.y = nrow;
			begin.x = 10;
		}
		if(stop.y > (nrow + trows))
		{
			stop.y = nrow + trows;
			stop.x = 57;
		}
		begin.y = begin.y - nrow + 1;
		stop.y = stop.y - nrow + 1;

		for(index = begin;index.y <= stop.y;index.y++)
		{
			if(index.y == begin.y)
			{	
				rt1.top = index.y * 2 * m_nullWidth ;
				rt1.bottom = (index.y + 1) * 2 * m_nullWidth ;
				rt1.left = index.x * m_nullWidth;
				if(index.y == stop.y)
				{
					rt1.right = stop.x * m_nullWidth;
				}
				else
				{
					rt1.right = 57 * m_nullWidth;
				}
			}
			else if(index.y == stop.y && index.y > begin.y)
			{
				rt1.top = index.y * 2 * m_nullWidth ;
				rt1.bottom = (index.y + 1) * 2 * m_nullWidth ;
				rt1.left = 10 * m_nullWidth;
				rt1.right = stop.x  * m_nullWidth;
			}
			else
			{
				rt1.top = index.y  * 2 * m_nullWidth ;
				rt1.bottom = (index.y + 1) * 2 * m_nullWidth ;
				rt1.left = 10 * m_nullWidth;
				rt1.right = 57 * m_nullWidth;
			}
			dc.Rectangle(&rt1);
		}
		pen_sl.DeleteObject();
	}

	/////////////////////////////////////////////////draw header
	dc.SetTextColor(RGB(110,0,0));
//	dc.Rectangle(0,0,rect.right, 2 * m_nullWidth);
	dc.Rectangle(0,0,rect.right,rect.bottom	);

	char header[]=" offset   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F    ASCII";
	y += 1;
    for(i=0;i<(sizeof(header)-1);i++)
	{
		dc.TextOut(x,y,(LPCTSTR)&header[i],1);
		x += m_nullWidth;
	}
	y = 0;
	y += 2 * m_nullWidth;
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(RGB(0,122,70));
	bool colorselect = true;
	for(i=0;i<trows;i++)      //draw map
	{
		if((i+nrow) > (map_row-1))
			break;
		/////////////////////////////////////////////draw address area and value area
		if((nrow+i) % 32 == 0 && (nrow + i) != 0) // snap to 1 Sector 512Byte
		{
			dc.SelectObject(&pen_line);
			dc.MoveTo(0,y);
			dc.LineTo(rect.right,y);	
		}
		colorselect =true;
		for(int j=0;j<59;j++)
		{
			x=j*m_nullWidth;
			if((j-10)%12==0)
				colorselect=!colorselect;
			if(colorselect)
				dc.SetTextColor(RGB(0,122,70));
			else
				dc.SetTextColor(RGB(110,0,0));
			dc.TextOut(x,y,(LPCTSTR)&map[i+nrow][j],1);
		}
		//////////////////////////////////////////////draw ASCII area
		temp.GetBufferSetLength(16);
		memcpy(temp.GetBuffer(1),(LPCTSTR)&map[i+nrow][59],16);
		temp.ReleaseBuffer();
		x=59*m_nullWidth;
		dc.TextOut(x,y,temp);
		y += 2 * m_nullWidth;
	}

	pdc.BitBlt(0, 0, rect.Width(), rect.Height(), &dc, 0, 0, SRCCOPY);

	DestroyCaret();
	CreateSolidCaret(m_nullWidth, 15);
	SetCaretPos(pt_c);
	ShowCaret();

	::SetScrollPos(this->m_hWnd, SB_VERT, nrow, TRUE);
}

void CHexEdit::PosCal(CPoint* point)
{
	CPoint pt;
	pt.x = point->x;
	pt.y = point->y;

	if(((pt.x / m_nullWidth) % 3) == 0 )
		pt.x += m_nullWidth;

	pt.x = pt.x - (pt.x % m_nullWidth);
	pt.y = pt.y - (pt.y % (m_nullWidth * 2)); 

	if(pt.x < 10 * m_nullWidth) //address area
	{
		if(nrow == 0 && pt.y / (2 * m_nullWidth) <= 1)
			pt.x = 10 * m_nullWidth;
		else
		{
			pt.x = 56 * m_nullWidth;
			pt.y -= 2*m_nullWidth;
		}
	}
	else if(pt.x >= 57 * m_nullWidth) //ASSIC area
	{
		if((nrow + trows) >= map_row && pt.y / (2 * m_nullWidth) == (rect.bottom / (2 * m_nullWidth) -1))
			pt.x = 56 * m_nullWidth;
		else
		{
			pt.x = 10 * m_nullWidth;
			pt.y += 2*m_nullWidth;
		}
	}
	
	if(pt.y < (2 * m_nullWidth))
	{
		pt.y = 2 * m_nullWidth;
		if(nrow > 0)
			nrow--;
	}
	else if(pt.y > (rect.bottom-2 * m_nullWidth))
	{
		pt.y -= 2*m_nullWidth;
		if((nrow+trows) < map_row)
			nrow++;
	}
	point->x = pt.x;
	point->y = pt.y;
}

void CHexEdit::SetContent(void* data,long len)
{
	unsigned char ch,a,b;
	unsigned char buffer[128 * 1024];;
	int i,j,index,temp_long,rem;
	if( len > 128 *1024)
	{
		AfxMessageBox("Not enough buffer!");
		return;
	}
	buffer_len = len;
	memcpy(buffer,data,len);

	rem = len % 16;
	map_row = len/16;
	if(rem != 0)
		map_row++;
	ZeroMemory(map,sizeof(map));
	index = 0;

	for(i=0;i<map_row;i++)
	{
		temp_long=i * 16;;
		map[i][0]=hextable[(temp_long >> 28) & 0xF]; // address area
		map[i][1]=hextable[(temp_long >> 24) & 0xF];
		map[i][2]=hextable[(temp_long >> 20) & 0xF];
		map[i][3]=hextable[(temp_long >> 16) & 0xF];
		map[i][4]=hextable[(temp_long >> 12) & 0xF];
		map[i][5]=hextable[(temp_long >> 8) & 0xF];
		map[i][6]=hextable[(temp_long >> 4) & 0xF];
		map[i][7]=hextable[(temp_long >> 0) & 0xF];
		map[i][8]=0x20;
		map[i][9]=0x20;
		map[i][57]=0x20;
		map[i][58]=0x20;
		temp_long = 10;  //hex index
		rem = 59;		//ASSIC index
		for(j=0;j<16;j++)
		{
			if(index >= len)
			{
				ch=0x20;
				a=0x20;
				b=0x20;
			}
			else
			{
				ch=buffer[index];
				index ++;
				a=hextable[(ch >> 4) & 0xF];
				b=hextable[(ch >> 0) & 0xF];
			}
			if(ch == 0x00)
				ch = 0x2E;
			map[i][rem++]=ch;		//assic area
			map[i][temp_long++]=a;	//hex area
			map[i][temp_long++]=b;
			map[i][temp_long++]=0x20;
		}
		map[i][33]='-';
	}
	
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax =map_row;
	si.nPage =trows;
	si.nPos =0;
	::SetScrollInfo(this->m_hWnd, SB_VERT, &si, TRUE);
//	if(si.nMax> (int)si.nPage)
		::EnableScrollBar(this->m_hWnd, SB_VERT, ESB_ENABLE_BOTH);
		RedrawWindow();
}

void CHexEdit::GetContent(void* data, DWORD len)
{
	unsigned char buffer[128 * 1024];
	unsigned char ch,chr;
	int index ;
	int total = 0;
	
	for(int i=0;i<map_row;i++)
	{
		index = 10;
		for(int j=0;j<16;j++)
		{
			ch = map[i][index++];	//1
			if( ch >= 0x41)
				ch = ch + 0x0A - 0x41;
			ch = ch << 4;

			chr = map[i][index++]; //2
			if( chr >= 0x41)
				chr = chr + 0x0A - 0x41;
			chr &= 0x0F;

			ch |= chr;
			buffer[total++]=ch;

			index++;				 //3

			if(total == buffer_len)
				break;
		}
	}
	if(len > 32 * 1024)
		len = 32 * 1024;
	memcpy(data,buffer,len);
}

void CHexEdit::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch(nSBCode)
	{
		case SB_LINEDOWN:
			nrow++;
			break;
		
		case SB_LINEUP:
			nrow--;
			break;
	
		case SB_PAGEUP:
			nrow -= nPos;
			break;
		
		case SB_PAGEDOWN:
			nrow += nPos;
			break;
		
		case SB_THUMBTRACK:
			nrow = nPos;
			break;		
	}
	if(nrow < 0)
	{
		nrow = 0;
	}
	if((nrow + trows) >= map_row)
	{
		nrow = map_row - trows;
	}
	RedrawWindow();
	::SetScrollPos(this->m_hWnd, SB_VERT, nrow, TRUE);

//	CEdit::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CHexEdit::OnSetFocus(CWnd* pOldWnd)
{
	CEdit::OnSetFocus(pOldWnd);

	// TODO: 在此处添加消息处理程序代码
	Invalidate();
}

void CHexEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	char ch = nChar & 0xFF;
	if((ch >= 0x30 && ch <= 0x39) || (ch >= 0x41 && ch <= 0x46) || (ch >= 0x61 && ch <= 0x66))
	{
		if(ch >= 0x51)
			ch -= 0x30;
		int row = pt_c.y/(2 * m_nullWidth) - 1;
		int column = pt_c.x/m_nullWidth;
		row += nrow;
		map[row][column] = ch;

		int index = 10;
		char chr;
		for(int i =0; i <16;i++)  //remap assic area
		{
			ch = map[row][index++];
			if( ch >= 0x41)
				ch = ch + 0x0A - 0x41;
			ch = ch << 4;

			chr = map[row][index++];
			if( chr >= 0x41)
				chr = chr + 0x0A - 0x41;
			chr &= 0x0F;

			ch |= chr;
			if(ch == 0x00)
				ch = 0x2E;
			map[row][i + 59] = ch;
			index++;
		}

		pt_c.x += m_nullWidth;
	//	if((pt_c.x / m_nullWidth) % 3 == 0)
	//		pt_c.x += m_nullWidth;
	}
	else
	{
		if(nChar == VK_DOWN)
		{
			pt_c.y += 2*m_nullWidth;
		}
		else if(nChar == VK_UP)
		{
			pt_c.y -= 2*m_nullWidth;
		}
		else if(nChar == VK_RIGHT)
		{
			pt_c.x += m_nullWidth;
			if((pt_c.x / m_nullWidth) % 3 == 0)
				pt_c.x += m_nullWidth;
		}
		else if(nChar == VK_LEFT)
		{
			pt_c.x -= m_nullWidth;
			if((pt_c.x / m_nullWidth) % 3 == 0)
				pt_c.x -= m_nullWidth;
		}
		else if(nChar == VK_PRIOR )
		{
			nrow -= trows;
		}
		else if(nChar == VK_NEXT)
		{
			nrow += trows;
		}	
	}
	if(nrow < 0)
	{
		nrow = 0;
	}
	if((nrow + trows) >= map_row)
	{
		nrow = map_row - trows;
	}
	PosCal(&pt_c);
	Invalidate();
//	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CHexEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	pt_c = point;
	PosCal(&pt_c);
	Invalidate();
	CEdit::OnLButtonDblClk(nFlags, point);
}

void CHexEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	pt_c = point;
	PosCal(&pt_c);
	pt_begin = pt_c;

	lb_down=true;
	selected = false;
	Invalidate();
	CEdit::OnLButtonDown(nFlags, point);
}

void CHexEdit::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	lb_down = false;
	CEdit::OnLButtonUp(nFlags, point);
}

void CHexEdit::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(!lb_down)
		return;
	if( ! rect.PtInRect(point))
		return;

	pt_stop = point;
	PosCal(&pt_stop);
	pt_c = pt_stop;

	SelCal();

	selected = true;
	Invalidate();
//	CEdit::OnMouseMove(nFlags, point);
}

void CHexEdit::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	pt_c = point;
	PosCal(&pt_c);
	Invalidate();
	CEdit::OnRButtonDown(nFlags, point);
}

void CHexEdit::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	pt_c = point;
	PosCal(&pt_c);
	Invalidate();
//	CEdit::OnRButtonUp(nFlags, point);
}

BOOL CHexEdit::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(zDelta > 0)
	{
		if(nrow > 0)
		{
			nrow--;
			if(lb_down)
			{
				RA_begin--;;
			}
		}
	}
	else 
	{
		if((nrow + trows) < map_row)
		{
			nrow ++;
			if(lb_down)
			{
				RA_stop++;
			}
		}
	}
	Invalidate();
	return CEdit::OnMouseWheel(nFlags, zDelta, pt);
}

void CHexEdit::SelCal()
{
	RA_begin = pt_begin.y / (2 * m_nullWidth) + nrow - 1;
	CA_begin = pt_begin.x / m_nullWidth;
	RA_stop = pt_stop.y / (2 * m_nullWidth) + nrow - 1;
	CA_stop = pt_stop.x / m_nullWidth;

	int temp;
	if(RA_begin > RA_stop)  //judge direction
	{
		temp = RA_begin;
		RA_begin = RA_stop;
		RA_stop = temp;
		
		temp = CA_begin;
		CA_begin = CA_stop;
		CA_stop = temp;
	}
	else if(RA_begin == RA_stop)
	{
		if(CA_begin > CA_stop)
		{
			temp = CA_begin;
			CA_begin = CA_stop;
			CA_stop = temp;
		}
	}
}
