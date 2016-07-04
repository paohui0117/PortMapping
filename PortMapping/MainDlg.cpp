#include "stdafx.h"
#include "MainDlg.h"
#include "MyListItem.h"
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
	}

	pCur = m_PaintManager.FindSubControlByName(nullptr, L"connect_list");
	if (pCur->GetInterface(DUI_CTR_LIST))
	{
		m_pConnect_List = static_cast<CListUI*>(pCur);
	}
	m_pLeft_layout = m_PaintManager.FindSubControlByName(nullptr, L"left_layout");
	Test();
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

void CMainDlg::Test()
{
	CMyListItem* pItem = new CMyListItem;
	pItem->AddText(L"192.168.1.101");
	pItem->AddText(L"1234");
	pItem->AddText(L"13", true);
	pItem->SetClickTextFont(2);
	m_pMapping_List->Add(pItem);
}