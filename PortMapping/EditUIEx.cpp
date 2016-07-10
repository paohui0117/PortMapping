#include "stdafx.h"
#include "EditUIEx.h"
#include <olectl.h>
using namespace DuiLib;
DWORD CEditUIEx::m_nWaringCol = 0xffff0000;

class CEditWndEx : public CWindowWnd
{
public:
	CEditWndEx();

	void Init(CEditUIEx* pOwner);
	RECT CalPos();

	LPCTSTR GetWindowClassName() const;
	LPCTSTR GetSuperClassName() const;
	void OnFinalMessage(HWND hWnd);

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	void CheckText();
	LRESULT OnEditChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	void ShowToolTip(LPCWSTR strInfo);
	void HideToolTip();

protected:
	enum {
		DEFAULT_TIMERID = 2020,
	};

	CEditUIEx* m_pOwner;
	HBRUSH m_hBkBrush;
	bool m_bInit;
	bool m_bDrawCaret;
private://所有的CEditEx共用一个tooltip
	static TOOLINFO m_ToolTip;
	static HWND m_hwndTooltip;
	static CEditUIEx* m_plast;
};
TOOLINFO CEditWndEx::m_ToolTip;
HWND CEditWndEx::m_hwndTooltip = nullptr;
CEditUIEx* CEditWndEx::m_plast = nullptr;
CEditWndEx::CEditWndEx() : m_pOwner(NULL), m_hBkBrush(NULL), m_bInit(false), m_bDrawCaret(false)
{
}

void CEditWndEx::Init(CEditUIEx* pOwner)
{
	m_pOwner = pOwner;
	RECT rcPos = CalPos();
	UINT uStyle = WS_CHILD | ES_AUTOHSCROLL | pOwner->GetWindowStyls();
	if (m_pOwner->IsPasswordMode()) uStyle |= ES_PASSWORD;
	Create(m_pOwner->GetManager()->GetPaintWindow(), NULL, uStyle, 0, rcPos);

	HFONT hFont = NULL;
	int iFontIndex = m_pOwner->GetFont();
	if (iFontIndex != -1)
		hFont = m_pOwner->GetManager()->GetFont(iFontIndex);
	if (hFont == NULL)
		hFont = m_pOwner->GetManager()->GetDefaultFontInfo()->hFont;

	SetWindowFont(m_hWnd, hFont, TRUE);
	Edit_LimitText(m_hWnd, m_pOwner->GetMaxChar());
	if (m_pOwner->IsPasswordMode()) Edit_SetPasswordChar(m_hWnd, m_pOwner->GetPasswordChar());
	Edit_SetText(m_hWnd, m_pOwner->GetText());
	Edit_SetModify(m_hWnd, FALSE);
	SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
	Edit_Enable(m_hWnd, m_pOwner->IsEnabled() == true);
	Edit_SetReadOnly(m_hWnd, m_pOwner->IsReadOnly() == true);

	//Styls
	::ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
	::SetFocus(m_hWnd);
	if (m_pOwner->IsAutoSelAll()) {
		int nSize = GetWindowTextLength(m_hWnd);
		if (nSize == 0) nSize = 1;
		Edit_SetSel(m_hWnd, 0, nSize);
	}
	else {
		int nSize = GetWindowTextLength(m_hWnd);
		Edit_SetSel(m_hWnd, nSize, nSize);
	}
	if (!m_pOwner->m_bOK)
		ShowToolTip(m_pOwner->m_check_info.m_waring_info);
	m_bInit = true;
}

RECT CEditWndEx::CalPos()
{
	CDuiRect rcPos = m_pOwner->GetPos();
	RECT rcInset = m_pOwner->GetTextPadding();
	rcPos.left += rcInset.left;
	rcPos.top += rcInset.top;
	rcPos.right -= rcInset.right;
	rcPos.bottom -= rcInset.bottom;
	LONG lEditHeight = m_pOwner->GetManager()->GetFontInfo(m_pOwner->GetFont())->tm.tmHeight;
	if (lEditHeight < rcPos.GetHeight()) {
		rcPos.top += (rcPos.GetHeight() - lEditHeight) / 2;
		rcPos.bottom = rcPos.top + lEditHeight;
	}

	CControlUI* pParent = m_pOwner;
	RECT rcParent;
	while (pParent = pParent->GetParent()) {
		if (!pParent->IsVisible()) {
			rcPos.left = rcPos.top = rcPos.right = rcPos.bottom = 0;
			break;
		}
		rcParent = pParent->GetClientPos();
		if (!::IntersectRect(&rcPos, &rcPos, &rcParent)) {
			rcPos.left = rcPos.top = rcPos.right = rcPos.bottom = 0;
			break;
		}
	}

	return rcPos;
}

LPCTSTR CEditWndEx::GetWindowClassName() const
{
	return _T("EditWndEx");
}

LPCTSTR CEditWndEx::GetSuperClassName() const
{
	return WC_EDIT;
}

void CEditWndEx::OnFinalMessage(HWND hWnd)
{
	HWND cur = GetFocus();
	wchar_t name[20] = { 0 };
	GetClassNameW(cur, name, 19);
	if (_tccmp(name, L"EditUIEx") != 0)
		HideToolTip();
	m_pOwner->Invalidate();
	// Clear reference and die
	if (m_hBkBrush != NULL) ::DeleteObject(m_hBkBrush);
	m_pOwner->GetManager()->RemoveNativeWindow(hWnd);
	m_pOwner->m_pWindowEx = NULL;
	delete this;
}

LRESULT CEditWndEx::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
	if (uMsg == WM_CREATE) {
		m_pOwner->GetManager()->AddNativeWindow(m_pOwner, m_hWnd);
		if (m_pOwner->GetManager()->IsLayered()) {
			::SetTimer(m_hWnd, DEFAULT_TIMERID, ::GetCaretBlinkTime(), NULL);
		}
		bHandled = FALSE;
	}
	else if (uMsg == WM_KILLFOCUS) lRes = OnKillFocus(uMsg, wParam, lParam, bHandled);
	else if (uMsg == OCM_COMMAND) {
		if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) lRes = OnEditChanged(uMsg, wParam, lParam, bHandled);
		else if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE) {
			RECT rcClient;
			::GetClientRect(m_hWnd, &rcClient);
			::InvalidateRect(m_hWnd, &rcClient, FALSE);
		}
	}
	else if (uMsg == WM_KEYDOWN && TCHAR(wParam) == VK_RETURN) {
		m_pOwner->GetManager()->SendNotify(m_pOwner, DUI_MSGTYPE_RETURN);
	}
	else if (uMsg == OCM__BASE + WM_CTLCOLOREDIT || uMsg == OCM__BASE + WM_CTLCOLORSTATIC) {
		if (m_pOwner->GetManager()->IsLayered() && !m_pOwner->GetManager()->IsPainting()) {
			m_pOwner->GetManager()->AddNativeWindow(m_pOwner, m_hWnd);
		}
		DWORD clrColor = m_pOwner->GetNativeEditBkColor();
		if (clrColor == 0xFFFFFFFF) return 0;
		::SetBkMode((HDC)wParam, TRANSPARENT);
		DWORD dwTextColor = m_pOwner->GetTextColor();
		::SetTextColor((HDC)wParam, RGB(GetBValue(dwTextColor), GetGValue(dwTextColor), GetRValue(dwTextColor)));
		if (clrColor < 0xFF000000) {
			if (m_hBkBrush != NULL) ::DeleteObject(m_hBkBrush);
			RECT rcWnd = m_pOwner->GetManager()->GetNativeWindowRect(m_hWnd);
			HBITMAP hBmpEditBk = CRenderEngine::GenerateBitmap(m_pOwner->GetManager(), rcWnd, m_pOwner, clrColor);
			m_hBkBrush = ::CreatePatternBrush(hBmpEditBk);
			::DeleteObject(hBmpEditBk);
		}
		else {
			if (m_hBkBrush == NULL) {
				m_hBkBrush = ::CreateSolidBrush(RGB(GetBValue(clrColor), GetGValue(clrColor), GetRValue(clrColor)));
			}
		}
		return (LRESULT)m_hBkBrush;
	}
	else if (uMsg == WM_PAINT) {
		if (m_pOwner->GetManager()->IsLayered()) {
			m_pOwner->GetManager()->AddNativeWindow(m_pOwner, m_hWnd);
		}
		bHandled = FALSE;
	}
	else if (uMsg == WM_PRINT) {
		if (m_pOwner->GetManager()->IsLayered()) {
			lRes = CWindowWnd::HandleMessage(uMsg, wParam, lParam);
			if (m_pOwner->IsEnabled() && m_bDrawCaret) { // todo:判断是否enabled
				RECT rcClient;
				::GetClientRect(m_hWnd, &rcClient);
				POINT ptCaret;
				::GetCaretPos(&ptCaret);
				RECT rcCaret = { ptCaret.x, ptCaret.y, ptCaret.x, ptCaret.y + rcClient.bottom - rcClient.top };
				CRenderEngine::DrawLine((HDC)wParam, rcCaret, 1, 0xFF000000);
			}
			return lRes;
		}
		bHandled = FALSE;
	}
	else if (uMsg == WM_TIMER) {
		if (wParam == DEFAULT_TIMERID) {
			m_bDrawCaret = !m_bDrawCaret;
			RECT rcClient;
			::GetClientRect(m_hWnd, &rcClient);
			::InvalidateRect(m_hWnd, &rcClient, FALSE);
			return 0;
		}
		bHandled = FALSE;
	}
	else bHandled = FALSE;
	if (!bHandled) return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	return lRes;
}

LRESULT CEditWndEx::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	
	LRESULT lRes = ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	if ((HWND)wParam != m_pOwner->GetManager()->GetPaintWindow()) {
		::SendMessage(m_pOwner->GetManager()->GetPaintWindow(), WM_KILLFOCUS, wParam, lParam);
	}
	SendMessage(WM_CLOSE);
	return lRes;
}

void CEditWndEx::CheckText()
{
	bool bOK = m_pOwner->CheckContent();
	if (bOK)
		HideToolTip();
	else
		ShowToolTip(m_pOwner->m_check_info.m_waring_info);
	if (m_pOwner->m_bOK != bOK)
	{
		m_pOwner->m_bOK = bOK;
		m_pOwner->Invalidate();
	}
}

LRESULT CEditWndEx::OnEditChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if (!m_bInit) return 0;
	if (m_pOwner == NULL) return 0;
	// Copy text back
	int cchLen = ::GetWindowTextLength(m_hWnd) + 1;
	LPTSTR pstr = static_cast<LPTSTR>(_alloca(cchLen * sizeof(TCHAR)));
	ASSERT(pstr);
	if (pstr == NULL) return 0;
	::GetWindowText(m_hWnd, pstr, cchLen);
	m_pOwner->m_sText = pstr;
	m_pOwner->m_check_info.m_content = pstr;
	CheckText();
	m_pOwner->GetManager()->SendNotify(m_pOwner, DUI_MSGTYPE_TEXTCHANGED);
	if (m_pOwner->GetManager()->IsLayered()) m_pOwner->Invalidate();
	return 0;
}

void CEditWndEx::ShowToolTip(LPCWSTR strInfo)
{
	if (CEditWndEx::m_hwndTooltip && IsWindowVisible(m_hwndTooltip) &&
		m_plast == m_pOwner)
	{
		return;
	}
	m_plast = m_pOwner;
	::ZeroMemory(&m_ToolTip, sizeof(TOOLINFO));
	m_ToolTip.cbSize = sizeof(TOOLINFO);
	m_ToolTip.uFlags = TTF_IDISHWND | TTF_ABSOLUTE;
	m_ToolTip.hwnd = m_pOwner ? m_pOwner->GetManager()->GetPaintWindow() : m_hWnd;
	m_ToolTip.uId = (UINT_PTR)m_ToolTip.hwnd;
	m_ToolTip.hinst = CPaintManagerUI::GetInstance();
	m_ToolTip.lpszText = const_cast<LPTSTR>((LPCTSTR)strInfo);
	m_ToolTip.rect = m_pOwner->GetPos();
	if (m_hwndTooltip == NULL) {
		m_hwndTooltip = ::CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP |TTS_BALLOON, 
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
			m_ToolTip.hwnd, NULL, m_ToolTip.hinst, NULL);
		::SendMessage(m_hwndTooltip, TTM_ADDTOOL, 0, (LPARAM)&m_ToolTip);
	}
	::SendMessage(m_hwndTooltip, TTM_SETMAXTIPWIDTH, 0, 200);
	::SendMessage(m_hwndTooltip, TTM_SETTOOLINFO, 0, (LPARAM)&m_ToolTip);

	::SendMessage(m_hwndTooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_ToolTip);
	POINT pt = { m_ToolTip.rect.left, m_ToolTip.rect.top };
	ClientToScreen(m_ToolTip.hwnd, &pt);
	MoveWindow(m_hwndTooltip, pt.x + 30, pt.y - 45, 200, 50, true);

	
}

void CEditWndEx::HideToolTip()
{
	if (m_hwndTooltip != NULL)
	{
		::ShowWindow(m_hwndTooltip, false);
		::SendMessage(m_hwndTooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ToolTip);
		m_plast = nullptr;
	}
		
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CEditUIEx::CEditUIEx()
{
	m_pWindowEx = nullptr;
	m_bShowWaring = false;
}


CEditUIEx::~CEditUIEx()
{
}

LPCTSTR CEditUIEx::GetClass() const
{
	return L"EditUIEx";
}

LPVOID CEditUIEx::GetInterface(LPCTSTR pstrName)
{
	if (_tcscmp(pstrName, DUI_CTR_EDITEX) == 0) return static_cast<CEditUIEx*>(this);
	return CEditUI::GetInterface(pstrName);
}

void CEditUIEx::PaintBorder(HDC hDC)
{
	if (m_bShowWaring && !m_bOK)
	{
		DWORD oldFocusBorderColor = m_dwFocusBorderColor;
		DWORD oldBorderColor = m_dwBorderColor;
		m_dwFocusBorderColor = m_dwBorderColor = m_nWaringCol;
		CEditUI::PaintBorder(hDC);
		m_dwFocusBorderColor = oldFocusBorderColor;
		m_dwBorderColor = oldBorderColor;
	}
	else
		CEditUI::PaintBorder(hDC);
}

void CEditUIEx::DoEvent(DuiLib::TEventUI& event)
{
	if (!IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND) {
		if (m_pParent != NULL) m_pParent->DoEvent(event);
		else CLabelUI::DoEvent(event);
		return;
	}

	if (event.Type == UIEVENT_SETCURSOR && IsEnabled())
	{
		::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_IBEAM)));
		return;
	}
	if (event.Type == UIEVENT_WINDOWSIZE)
	{
		if (m_pWindowEx != NULL) m_pManager->SetFocusNeeded(this);
	}
	if (event.Type == UIEVENT_SCROLLWHEEL)
	{
		if (m_pWindowEx != NULL) return;
	}
	if (event.Type == UIEVENT_SETFOCUS && IsEnabled())
	{
		if (m_pWindowEx) return;
		m_pWindowEx = new CEditWndEx();
		ASSERT(m_pWindowEx);
		m_pWindowEx->Init(this);
		Invalidate();
	}
	if (event.Type == UIEVENT_KILLFOCUS && IsEnabled())
	{
		Invalidate();
	}
	if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK || event.Type == UIEVENT_RBUTTONDOWN)
	{
		if (IsEnabled()) {
			GetManager()->ReleaseCapture();
			if (IsFocused() && m_pWindowEx == NULL)
			{
				m_pWindowEx = new CEditWndEx();
				ASSERT(m_pWindowEx);
				m_pWindowEx->Init(this);
			}
			else if (m_pWindowEx != NULL)
			{
				if (!m_bAutoSelAll) {
					POINT pt = event.ptMouse;
					pt.x -= m_rcItem.left + m_rcTextPadding.left;
					pt.y -= m_rcItem.top + m_rcTextPadding.top;
					Edit_SetSel(*m_pWindowEx, 0, 0);
					::SendMessage(*m_pWindowEx, WM_LBUTTONDOWN, event.wParam, MAKELPARAM(pt.x, pt.y));
				}
			}
		}
		return;
	}
	if (event.Type == UIEVENT_MOUSEMOVE)
	{
		return;
	}
	if (event.Type == UIEVENT_BUTTONUP)
	{
		return;
	}
	if (event.Type == UIEVENT_CONTEXTMENU)
	{
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
		if (IsEnabled()) {
			m_uButtonState &= ~UISTATE_HOT;
			Invalidate();
		}
		return;
	}
	CLabelUI::DoEvent(event);
}

void CEditUIEx::SetText(LPCTSTR pstrText)
{
	m_sText = pstrText;
	if (m_pWindowEx != NULL) Edit_SetText(*m_pWindowEx, m_sText);
	if (m_bEnabled && m_bShowWaring)
	{
		m_check_info.m_content = pstrText;
		m_bOK = CheckContent();
	}
	Invalidate();
}

HWND CEditUIEx::GetNativeEditHWND() const
{
	if (m_pWindowEx)
		return *m_pWindowEx;
	return nullptr;
}

void CEditUIEx::SetPos(RECT rc, bool bNeedInvalidate)
{
	CControlUI::SetPos(rc, bNeedInvalidate);
	if (m_pWindowEx != NULL) {
		RECT rcPos = m_pWindowEx->CalPos();
		::SetWindowPos(m_pWindowEx->GetHWND(), NULL, rcPos.left, rcPos.top, rcPos.right - rcPos.left,
			rcPos.bottom - rcPos.top, SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void CEditUIEx::Move(SIZE szOffset, bool bNeedInvalidate)
{
	CControlUI::Move(szOffset, bNeedInvalidate);
	if (m_pWindowEx != NULL) {
		RECT rcPos = m_pWindowEx->CalPos();
		::SetWindowPos(m_pWindowEx->GetHWND(), NULL, rcPos.left, rcPos.top, rcPos.right - rcPos.left,
			rcPos.bottom - rcPos.top, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void CEditUIEx::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
	if (_tcscmp(pstrName, _T("waring")) == 0)
	{
		m_bShowWaring = (_tcscmp(pstrValue, _T("true")) == 0);
		if (m_bEnabled && m_bShowWaring)
		{
			m_check_info.m_content = m_sText;
			m_bOK = CheckContent();
		}
	}
	else if (_tcscmp(pstrName, _T("waringcolor")) == 0)
	{
		while (*pstrValue > _T('\0') && *pstrValue <= _T(' ')) pstrValue = ::CharNext(pstrValue);
		if (*pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
		LPTSTR pstr = NULL;
		DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
		SetWaringColor(clrColor);
	}
	CEditUI::SetAttribute(pstrName, pstrValue);
}

bool CEditUIEx::CheckContent()
{
	if (!OnCheck)
		return true;
	else
		return OnCheck(&m_check_info);
}