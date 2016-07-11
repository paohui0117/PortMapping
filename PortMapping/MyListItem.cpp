#include "stdafx.h"
#include "MyListItem.h"
using namespace DuiLib;

CDuiString GetFlowString(UINT32 nM, UINT32 nB)
{
	CDuiString str;
	UINT cur1 = nM >> 10; //G
	UINT cur2 = nB >> 10; //KB
	if (cur1 > 0)
		str.Format(L"%udG,%udM,%udKB", cur1, nM & 0x3ff, cur2);
	else if (nM > 0)
		str.Format(L"%udM,%udKB,%udB", nM, cur2, nB & 0x3ff);
	else if (cur2 > 0)
		str.Format(L"%udKB,%udB", cur2, nB & 0x3ff);
	else
		str.Format(L"%udB", nB);
	return str;
}

struct ListItemText
{
	CDuiString	strData;	//内容
	bool		bClick;		//是否可点击
	RECT*		pRc;		//文字范围
};
CMyListItem::CMyListItem() : m_nClickCor(0xff0000ff), m_nClictFont(-1)
{
	m_pCheck_box = new CCheckBoxUI;
}


CMyListItem::~CMyListItem()
{
	ClearText();
}

LPCTSTR CMyListItem::GetClass() const
{
	return L"CMyListItem";
}

LPVOID CMyListItem::GetInterface(LPCTSTR pstrName)
{
	if (_tcscmp(pstrName, DUI_CTR_MYLISTITEM) == 0) return static_cast<CMyListItem*>(this);
	return CListContainerElementUI::GetInterface(pstrName);
}

void CMyListItem::DoEvent(DuiLib::TEventUI& event)
{
	if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
		if (m_pOwner != NULL) m_pOwner->DoEvent(event);
		else CListContainerElementUI::DoEvent(event);
		return;
	}
	int nCur = -1;
	for (size_t i = 0; i < m_text_array.GetSize(); i++)
	{
		ListItemText* curData = (ListItemText*)m_text_array.GetAt(i);
		if (curData->pRc && PtInRect(curData->pRc, event.ptMouse))
		{
			nCur = i;
			break;
		}
	}
	if (event.Type == UIEVENT_MOUSEMOVE)
	{
		return;
	}
	if (event.Type == UIEVENT_BUTTONUP)
	{
		return;
	}
	if (event.Type == UIEVENT_SETCURSOR && nCur != -1)
	{
		
		::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
		return;	
	}
	if (event.Type == UIEVENT_BUTTONDOWN)
	{
		if (IsEnabled()) {
			m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMCLICK, nCur);
			Select();
			Invalidate();
		}
		return;
	}
	if (event.Type == UIEVENT_MOUSEENTER)
	{
		if (IsEnabled()) {
			m_uButtonState |= UISTATE_HOT;
			Invalidate();
		}
		return;
	}
	if (event.Type == UIEVENT_MOUSELEAVE)
	{
		if ((m_uButtonState & UISTATE_HOT) != 0) {
			m_uButtonState &= ~UISTATE_HOT;
			Invalidate();
		}
		return;
	}
	CListContainerElementUI::DoEvent(event);
}

SIZE CMyListItem::EstimateSize(SIZE szAvailable)
{
	//只在意高度
	if (m_pOwner == NULL) return CDuiSize(0, 0);
	TListInfoUI* pInfo = m_pOwner->GetListInfo();
	SIZE cXY = m_cxyFixed;
	if (cXY.cy == 0 && m_pManager != NULL) {
		cXY.cy = m_pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
		cXY.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
	}
	return cXY;
}

void CMyListItem::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
	DrawItemBk(hDC, m_rcItem);
	DrawItemText(hDC, m_rcItem);
	CContainerUI::DoPaint(hDC, rcPaint, pStopControl);
}

void CMyListItem::DrawItemText(HDC hDC, const RECT& rcItem)
{
	TListInfoUI* pInfo = m_pOwner->GetListInfo();
	if (!pInfo)
		return;
	RECT curRect;
	DWORD iTextColor = pInfo->dwTextColor;
	int nFont = pInfo->nFont;
	int nLinks = 0;
	ListItemText* curData = nullptr;
	for (size_t i = 0; i < pInfo->nColumns && i < m_text_array.GetSize(); i++)
	{
		iTextColor = pInfo->dwTextColor;
		nFont = pInfo->nFont;
		//循环画每一列的内容文字
		curRect = pInfo->rcColumn[i];
		curRect.top = rcItem.top;
		curRect.bottom = rcItem.bottom;
		if (curRect.left > rcItem.right)//超出范围
			break;
		curData = ((ListItemText*)m_text_array.GetAt(i));
		if ((m_uButtonState & UISTATE_HOT) != 0 ) {
			iTextColor = pInfo->dwHotTextColor;
		}
		if (IsSelected()) {
			iTextColor = pInfo->dwSelectedTextColor;
		}
		if (!IsEnabled()) {
			iTextColor = pInfo->dwDisabledTextColor;
		}
		RECT rcText = curRect;
		if (i == 0)//给checkbox留空间
			rcText.left += 10;
		rcText.left += pInfo->rcTextPadding.left;
		rcText.right -= pInfo->rcTextPadding.right;
		rcText.top += pInfo->rcTextPadding.top;
		rcText.bottom -= pInfo->rcTextPadding.bottom;
		if (curData->bClick)
		{
			iTextColor = m_nClickCor;
			if (m_nClictFont != -1)
				nFont = m_nClictFont;
			GetTextRect(i, curData->pRc, rcItem);
		}
			
		if (pInfo->bShowHtml)
			CRenderEngine::DrawHtmlText(hDC, m_pManager, rcText, curData->strData, iTextColor, \
				NULL, NULL, nLinks, DT_SINGLELINE | pInfo->uTextStyle);
		else
			CRenderEngine::DrawText(hDC, m_pManager, rcText, curData->strData, iTextColor, \
				nFont, DT_SINGLELINE | pInfo->uTextStyle);
	}
}

void CMyListItem::DoInit()
{
	m_pCheck_box->SetFloat();
	m_pCheck_box->SetAttribute(L"pos", L"1,2,17,19");
	m_pCheck_box->SetNormalImage(L"unchecked.png");
	m_pCheck_box->SetSelectedImage(L"checked.png");
	this->Add(m_pCheck_box);
}

UINT CMyListItem::GetControlFlags() const
{
	return UIFLAG_SETCURSOR;
}

int CMyListItem::AddText(const DuiLib::CDuiString& strData, bool bClick)
{
	ListItemText* pCur = new ListItemText;
	pCur->strData = strData;
	pCur->bClick = bClick;
	if (pCur->bClick)
		pCur->pRc = new RECT;
	else
		pCur->pRc = nullptr;
	if (m_text_array.Add(pCur))
		return m_text_array.GetSize() - 1;
	//出错  释放内存
	if (pCur->pRc)
		delete pCur->pRc;
	delete pCur;
	return -1;
}

bool CMyListItem::SetText(int nIndex, const DuiLib::CDuiString& strData)
{
	ListItemText* pCur = (ListItemText*)m_text_array.GetAt(nIndex);
	if (pCur)
	{
		pCur->strData = strData;
		return true;
	}
	return false;
}

bool CMyListItem::SetClick(int nIndex, bool bClick)
{
	ListItemText* pCur = (ListItemText*)m_text_array.GetAt(nIndex);
	if (pCur)
	{
		pCur->bClick = bClick;
		if (pCur->bClick && pCur->pRc == nullptr)
			pCur->pRc = new RECT;
		if (!pCur->bClick && pCur->pRc)
		{
			delete pCur->pRc;
			pCur->pRc = nullptr;
		}
		return true;
	}
	return false;
}

bool CMyListItem::DeleteText(int nIndex)
{
	ListItemText* pCur = (ListItemText*)m_text_array.GetAt(nIndex);
	if (pCur )
	{
		if (pCur->pRc)
			delete pCur->pRc;
		delete pCur;
	}
	return m_text_array.Remove(nIndex);
}

bool CMyListItem::InsetText(int nIndex, const DuiLib::CDuiString& strData, bool bClick)
{
	ListItemText* pCur = new ListItemText;
	pCur->strData = strData;
	pCur->bClick = bClick;
	if (pCur->bClick)
		pCur->pRc = new RECT;
	else
		pCur->pRc = nullptr;
	if (m_text_array.InsertAt(nIndex, pCur))
		return true;
	if (pCur->pRc)
		delete pCur->pRc;
	delete pCur;
	return false;
}

int CMyListItem::ClearText()
{
	int nsize = m_text_array.GetSize();
	for (size_t i = 0; i < nsize; i++)
	{
		ListItemText* pCur = (ListItemText*)m_text_array.GetAt(i);
		if (pCur)
		{
			if (pCur->bClick && pCur->pRc)
				delete pCur->pRc;
			delete pCur;
		}
	}
	m_text_array.Empty();
	return nsize;
}

bool CMyListItem::GetTextRect(int nIndex, RECT* prc, const RECT& rcitem)
{
	if (!prc)
		return false;
	TListInfoUI* pInfo = m_pOwner->GetListInfo();
	if (!pInfo || nIndex < 0 || nIndex > m_text_array.GetSize() - 1 || nIndex > pInfo->nColumns - 1)
		return false;
	ListItemText* pCur = (ListItemText*)m_text_array.GetAt(nIndex);
	int nFont = pInfo->nFont;
	if (pCur->bClick && m_nClictFont != -1)
		nFont = m_nClictFont;
	*prc = pInfo->rcColumn[nIndex];
	prc->top = rcitem.top;
	prc->bottom = rcitem.bottom;
	int nwidth = prc->right - prc->left;
	if (pInfo->bShowHtml) {
		int nLinks = 0;
		CRenderEngine::DrawHtmlText(m_pManager->GetPaintDC(), m_pManager, *prc, pCur->strData, 0, NULL, NULL, nLinks, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle );
	}
	else {
		CRenderEngine::DrawText(m_pManager->GetPaintDC(), m_pManager, *prc, pCur->strData, 0, nFont, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle);
	}
	//默认是居中显示的
	nwidth -= (prc->right - prc->left);
	if (nwidth > 0)
	{
		nwidth /= 2;
		prc->left += nwidth;
		prc->right += nwidth;
	}
	return true;
}
//////////////////////////////////////////////////////////////////
CMappingListItem::CMappingListItem(MappingInfo* pInfo)
{
	m_pInfo = pInfo;
}

CMappingListItem::~CMappingListItem()
{
}

void CMappingListItem::DoInit()
{
	//添加状态图标
	CControlUI* pCtrl = new CControlUI;
	pCtrl->SetAttribute(L"pos", L"18,5,30,17");
	pCtrl->SetFloat();
	SIZE rs = { 12, 12 };
	pCtrl->SetBorderRound(rs);
	pCtrl->SetBkColor(0xffececec);
	Add(pCtrl);
	CMyListItem::DoInit();
	if (!m_pInfo)
		return;
	AddText(L"stop");
	AddText(L"0", true);
	AddText(m_pInfo->bTCP? L"TCP":L"UDP");
	AddText(L"0B");
	AddText(L"0B");
}

LPCTSTR CMappingListItem::GetClass() const
{
	return L"MappingListItem";
}

LPVOID CMappingListItem::GetInterface(LPCTSTR pstrName)
{
	if (_tcscmp(pstrName, DUI_CTR_MAPPINGLISTITEM) == 0) return static_cast<CMyListItem*>(this);
	return CMyListItem::GetInterface(pstrName);
}

void CMappingListItem::InitStringList(const DuiLib::CDuiString& strAgentIP, const DuiLib::CDuiString& strAgentPort,const DuiLib::CDuiString& strServerIP, const DuiLib::CDuiString& strServerPort)
{
	ClearText();
	AddText(strAgentIP);
	AddText(strAgentPort);
	AddText(strServerIP);
	AddText(strServerPort);
}

bool CMappingListItem::Start(bool bSelect)
{
	return false;
}

bool CMappingListItem::Stop(bool bSelect)
{
	return false;
}

bool CMappingListItem::Delete(bool bSelect)
{
	return false;
}

void CMappingListItem::Updata(bool bforce)
{
	if (!m_pInfo)
		return;
	if (!bforce && m_pInfo->nState == 0)
		return;
	//状态
	if (m_pInfo->nState == 0 )
		SetText(4, L"停止");
	else if (m_pInfo->nState & MAPPING_START)
		SetText(4, L"开始");
	else if (m_pInfo->nState & MAPPING_FAIL)
		SetText(4, L"出错");
	//数量
	CDuiString str;
	str.Format(L"%d", m_pInfo->nConnect);
	SetText(5, str);
	//客户发送   client——>agent——>server
	str = GetFlowString(m_pInfo->nTotalFromClientM, m_pInfo->nTotalFromClientB);
	SetText(7, str);
	//服务端发送   server——>agent——>client
	str = GetFlowString(m_pInfo->nTotalFromServerM, m_pInfo->nTotalFromServerB);
	SetText(8, str);
}