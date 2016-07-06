#include "stdafx.h"
#include "MainDlg.h"
#include "MyListItem.h"
#include "UIMenu.h"
CMainDlg::CMainDlg() :
	m_pLeft_hide(nullptr), m_pBottom_hide(nullptr), m_pLeft_layout(nullptr),
	m_pMapping_List(nullptr), m_pConnect_List(nullptr), m_pMenu_hide(nullptr)
{
}


CMainDlg::~CMainDlg()
{
}

CDuiString CMainDlg::GetSkinFolder()
{
	return L"";
}
//返回xml文件资源的资源ID
CDuiString CMainDlg::GetSkinFile()
{
	return L"skin.xml";
}

LPCTSTR CMainDlg::GetWindowClassName(void) const
{
	return L"CMainDlg_Duilib";
}

void CMainDlg::InitWindow()
{
	//获取控件指针
	CControlUI* pCur = nullptr;
	pCur = m_PaintManager.FindSubControlByName(nullptr, L"btn_left");
	if (pCur->GetInterface(DUI_CTR_BUTTON))
	{
		m_pLeft_hide = static_cast<CButtonUI*>(pCur);
		m_pLeft_hide->OnNotify += MakeDelegate(this, &CMainDlg::ButtonNotify);
	}


	pCur = m_PaintManager.FindSubControlByName(nullptr, L"btn_bottom");
	if (pCur->GetInterface(DUI_CTR_BUTTON))
	{
		m_pBottom_hide = static_cast<CButtonUI*>(pCur);
		m_pBottom_hide->OnNotify += MakeDelegate(this, &CMainDlg::ButtonNotify);
	}

	pCur = m_PaintManager.FindSubControlByName(nullptr, L"menubtn");
	if (pCur->GetInterface(DUI_CTR_BUTTON))
	{
		m_pMenu_hide = static_cast<CButtonUI*>(pCur);
		m_pMenu_hide->OnNotify += MakeDelegate(this, &CMainDlg::ButtonNotify);
	}

	pCur = m_PaintManager.FindSubControlByName(nullptr, L"mapping_list");
	if (pCur->GetInterface(DUI_CTR_LIST))
	{
		m_pMapping_List = static_cast<CListUI*>(pCur);
		m_pMapping_List->OnNotify += MakeDelegate(this, &CMainDlg::ListNotify);
	}

	pCur = m_PaintManager.FindSubControlByName(nullptr, L"connect_list");
	if (pCur->GetInterface(DUI_CTR_LIST))
	{
		m_pConnect_List = static_cast<CListUI*>(pCur);
		m_pConnect_List->OnNotify += MakeDelegate(this, &CMainDlg::ListNotify);
	}
	m_pLeft_layout = m_PaintManager.FindSubControlByName(nullptr, L"left_layout");

	if (m_PaintManager.GetRoot())
	{
		m_PaintManager.GetRoot()->OnNotify += MakeDelegate(this, &CMainDlg::RootNotify);
	}
	Test();
}
bool CMainDlg::RootNotify(void* p)
{
	TNotifyUI* pNotify = (TNotifyUI*)p;
	if (!pNotify)
		return false;
	if (pNotify->sType == DUI_MSGTYPE_MENUITEM_INIT)
	{
		OnMenuItemInit((CMenuElementUI*)pNotify->wParam, pNotify->lParam);
	}
	else if (pNotify->sType == DUI_MSGTYPE_MENUITEM_CLICK)
	{
		OnMenuItemClick(LPCWSTR(pNotify->wParam), pNotify->lParam);
	}
	return false;
}
LRESULT CMainDlg::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
	//屏蔽escape键
	if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)
	{
		bHandled = true;
		return S_OK;
	}
	return WindowImplBase::MessageHandler(uMsg, wParam, lParam, bHandled);
}

LRESULT CMainDlg::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (uMsg == WM_USER_MENUITEM_CLICK)
	{
		m_PaintManager.SendNotify(m_PaintManager.GetRoot(), DUI_MSGTYPE_MENUITEM_CLICK, wParam, lParam);
		bHandled = true;
		return S_OK;
	}
	bHandled = false;
	return 0;
}

void CMainDlg::OnMenuItemInit(CMenuElementUI* pMenuItem, LPARAM l_param)
{
	if (!pMenuItem)
		return;
	int a = 10;
}

void CMainDlg::OnMenuItemClick(LPCWSTR pName, LPARAM l_param)
{
	if (!pName)
		return;
	MessageBox(m_hWnd, pName, L"菜单单击", MB_OK);
	int a = 10;
}

bool CMainDlg::ButtonNotify(void* pNotify)
{
	TNotifyUI* pNotifyUI = (TNotifyUI*)pNotify;
	if (!pNotifyUI)
		return true;
	if (pNotifyUI->sType == DUI_MSGTYPE_CLICK)//按钮单击
	{
		bool bvisible;
		if (pNotifyUI->pSender == m_pLeft_hide)
		{
			bvisible = m_pLeft_layout->IsVisible();
			m_pLeft_layout->SetVisible(!bvisible);
			m_pLeft_hide->SetText(!bvisible ? L"◀" : L"▶");
		}
		else if (pNotifyUI->pSender == m_pBottom_hide)
		{
			bvisible = m_pConnect_List->IsVisible();
			m_pConnect_List->SetVisible(!bvisible);
			m_pBottom_hide->SetText(!bvisible ? L"▼" : L"▲");
		}
		else if (pNotifyUI->pSender == m_pMenu_hide)
		{

		}
	}
	return true;
}

bool CMainDlg::ListNotify(void* pNotify)
{
	TNotifyUI* pNotifyUI = (TNotifyUI*)pNotify;
	if (!pNotifyUI || !pNotifyUI->pSender)
		return true;
	if (pNotifyUI->sType == DUI_MSGTYPE_MENU)//菜单弹出消息
	{
		CListUI* pList = (CListUI*)pNotifyUI->pSender;
		RECT rcHead = pList->GetHeader()->GetPos();
		if (PtInRect(&rcHead, pNotifyUI->ptMouse))
			return true;
		POINT pt = pNotifyUI->ptMouse;
		ClientToScreen(m_hWnd, &pt);
		if (pList == m_pMapping_List)
		{
			STRINGorID xml(L"mappingmenu.xml");
			ShowMenu(&m_PaintManager, xml, pt);
		}
		else if (pList == m_pConnect_List)
		{
			STRINGorID xml(L"connectmenu.xml");
			ShowMenu(&m_PaintManager, xml, pt);
		}
	}
	return true;
}

void CMainDlg::Test()
{
	CMyListItem* pItem = new CMyListItem;
	pItem->AddText(L"192.168.1.101");
	pItem->AddText(L"1234");
	pItem->AddText(L"13", true);
	pItem->SetClickTextFont(2);
	m_pMapping_List->Add(pItem);
}