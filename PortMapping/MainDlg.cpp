#include "stdafx.h"
#include "MainDlg.h"
#include "UIMenu.h"
#include "EditUIEx.h"
#define MAIN_UPDATA_TIMER 1234
CMainDlg::CMainDlg() :
	m_pLeft_hide(nullptr), m_pBottom_hide(nullptr), m_pLeft_layout(nullptr),
	m_pMapping_List(nullptr), m_pConnect_List(nullptr), m_pMenu_hide(nullptr),
	m_pEdit_agent_port(nullptr), m_pEdit_server_port(nullptr), m_pEdit_server_ip(nullptr),
	m_pCmb_protocol(nullptr), m_pBtn_ADD(nullptr), m_pCur_mapping(nullptr),
	m_pCmb_agent_ip(nullptr)
{
	m_pLibuv = new CLibuvAdapter;
	m_pregex_IP = new wregex(L"^((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)($|(?!\\.$)\\.)){4}$");
}


CMainDlg::~CMainDlg()
{
	delete m_pLibuv;
	delete m_pregex_IP;
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

CControlUI* CMainDlg::CreateControl(LPCTSTR pstrClass)
{
	if (wcsncmp(pstrClass, DUI_CTR_EDITEX, wcslen(pstrClass)) == 0)
	{
		return new CEditUIEx;
	}
	return nullptr;
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
	//映射列表
	pCur = m_PaintManager.FindSubControlByName(nullptr, L"mapping_list");
	if (pCur->GetInterface(DUI_CTR_LIST))
	{
		m_pMapping_List = static_cast<CListUI*>(pCur);
		m_pMapping_List->OnNotify += MakeDelegate(this, &CMainDlg::ListNotify);
	}
	//链接列表
	pCur = m_PaintManager.FindSubControlByName(nullptr, L"connect_list");
	if (pCur->GetInterface(DUI_CTR_LIST))
	{
		m_pConnect_List = static_cast<CListUI*>(pCur);
		m_pConnect_List->OnNotify += MakeDelegate(this, &CMainDlg::ListNotify);
	}
	m_pLeft_layout = m_PaintManager.FindSubControlByName(nullptr, L"left_layout");
	//add按钮
	pCur = m_PaintManager.FindSubControlByName(m_pLeft_layout, L"add_mapping");
	if (pCur->GetInterface(DUI_CTR_BUTTON))
	{
		m_pBtn_ADD = static_cast<CButtonUI*>(pCur);
		m_pBtn_ADD->OnNotify += MakeDelegate(this, &CMainDlg::ButtonNotify);
	}
	//端口编辑控件
	pCur = m_PaintManager.FindSubControlByName(m_pLeft_layout, L"local_port");
	if (pCur->GetInterface(DUI_CTR_EDITEX))
	{
		m_pEdit_agent_port = static_cast<CEditUIEx*>(pCur);
		m_pEdit_agent_port->OnCheck += MakeDelegate(this, &CMainDlg::CheckPort);
		m_pEdit_agent_port->SetText(m_pEdit_agent_port->GetText());
	}
	pCur = m_PaintManager.FindSubControlByName(m_pLeft_layout, L"mapping_port");
	if (pCur->GetInterface(DUI_CTR_EDITEX))
	{
		m_pEdit_server_port = static_cast<CEditUIEx*>(pCur);
		m_pEdit_server_port->OnCheck += MakeDelegate(this, &CMainDlg::CheckPort);
		m_pEdit_server_port->SetText(m_pEdit_server_port->GetText());
	}
	//IP
	pCur = m_PaintManager.FindSubControlByName(m_pLeft_layout, L"mapping_ip");
	if (pCur->GetInterface(DUI_CTR_EDITEX))
	{
		m_pEdit_server_ip = static_cast<CEditUIEx*>(pCur);
		m_pEdit_server_ip->OnCheck += MakeDelegate(this, &CMainDlg::CheckIP);
		m_pEdit_server_ip->SetText(m_pEdit_server_ip->GetText());
	}

	pCur = m_PaintManager.FindSubControlByName(m_pLeft_layout, L"protocol");
	if (pCur->GetInterface(DUI_CTR_COMBO))
	{
		m_pCmb_protocol = static_cast<CComboUI*>(pCur);
		m_pCmb_protocol->SelectItem(0);
	}
	pCur = m_PaintManager.FindSubControlByName(m_pLeft_layout, L"local_ip");
	if (pCur->GetInterface(DUI_CTR_COMBO))
	{
		m_pCmb_agent_ip = static_cast<CComboUI*>(pCur);
		m_pCmb_agent_ip->SelectItem(0);
		GetLocalIP();
	}
	
	if (m_PaintManager.GetRoot())
	{
		m_PaintManager.GetRoot()->OnNotify += MakeDelegate(this, &CMainDlg::RootNotify);
		m_PaintManager.SetTimer(m_PaintManager.GetRoot(), MAIN_UPDATA_TIMER, 600);
	}
	
}
bool CMainDlg::CheckPort(void* p)
{
	CheckInfo* pInfo = (CheckInfo*)p;
	if (!pInfo)
		return true;
	for (size_t i = 0; i < pInfo->m_content.GetLength(); i++)
	{
		if (pInfo->m_content[i] < '0' || pInfo->m_content[i] > '9')
		{
			pInfo->m_waring_info = L"端口应该为1-65535";
			m_pBtn_ADD->SetEnabled(false);
			return false;
		}
			
	}
	int nport = _wtoi(pInfo->m_content);
	if (nport < 1 || nport > 65535)
	{
		pInfo->m_waring_info = L"端口应该为1-65535";
		m_pBtn_ADD->SetEnabled(false);
		return false;
	}
	m_pBtn_ADD->SetEnabled(CheckAllInfo());
	return true;
}

bool CMainDlg::CheckIP(void* p)
{
	CheckInfo* pInfo = (CheckInfo*)p;
	if (!pInfo)
		return true;
	if (!regex_match(pInfo->m_content.GetData(), *m_pregex_IP))
	{
		pInfo->m_waring_info = L"IP格式错误！";
		m_pBtn_ADD->SetEnabled(false);
		return false;
	}
	m_pBtn_ADD->SetEnabled(CheckAllInfo());
	return true;
}

void CMainDlg::GetLocalIP()
{
	if (!m_pLibuv)
		return;
	if (m_pLibuv->GetLocalIP(m_vecLocalIP))
	{
		for (size_t i = 0; i < m_vecLocalIP.size(); i++)
		{
			CListLabelElementUI* pItem = new CListLabelElementUI();
			pItem->SetText(m_vecLocalIP[i].c_str());
			m_pCmb_agent_ip->Add(pItem);
		}
	}
	m_pCmb_agent_ip->SelectItem(0);
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
	else if (pNotify->sType == DUI_MSGTYPE_TIMER)
	{
		if (MAIN_UPDATA_TIMER == pNotify->wParam)
			UpDataList();
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

void CMainDlg::UpDataList()
{
	if (!m_pMapping_List)
		return;
	CControlUI* pCtrl = nullptr;
	CMappingListItem *pMapLiem = nullptr;
	for (size_t i = 0; i < m_pMapping_List->GetCount(); i++)
	{
		pCtrl = m_pMapping_List->GetItemAt(i);
		if (!pCtrl)
			continue;
		pMapLiem = static_cast<CMappingListItem *>(pCtrl->GetInterface(DUI_CTR_MAPPINGLISTITEM));
		if (pMapLiem)
			pMapLiem->Updata();
	}
	if (!m_pConnect_List)
		return;
	CConnectListItem* pConnectItem = nullptr;
	for (size_t i = 0; i < m_pConnect_List->GetCount(); i++)
	{
		pCtrl = m_pConnect_List->GetItemAt(i);
		if (!pCtrl)
			continue;
		pConnectItem = static_cast<CConnectListItem *>(pCtrl->GetInterface(DUI_CTR_CONNECTLISTITEM));
		if (pConnectItem)
			pConnectItem->Updata();
	}
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



void CMainDlg::OnAddClick()
{
	if (!m_pLibuv)
		return;
	CDuiString strAgentIP = m_pCmb_agent_ip->GetText();
	CDuiString strServerIP = m_pEdit_server_ip->GetText();
	CDuiString strAgentPort = m_pEdit_agent_port->GetText();
	CDuiString strServerPort = m_pEdit_server_port->GetText();
	CDuiString strProtocol = m_pCmb_protocol->GetText();
	bool bTcp = strProtocol == L"TCP";
	int nerr = 0;
	MappingInfo* pInfo = m_pLibuv->AddMapping(strAgentIP, strAgentPort, strServerIP, strServerPort,
		bTcp, nerr);
	if (nerr == 1)
	{
		MessageBox(m_hWnd, L"映射已经存在", L"提示", MB_OK);
		return;
	}
	else if (nerr == 2)
	{
		MessageBox(m_hWnd, L"本地地址与映射地址不能相同！", L"提示", MB_OK);
		return;
	}
	else if (nerr == 0)
	{
		CMappingListItem* pItem = new CMappingListItem(pInfo);
		pItem->InitStringList(strAgentIP, strAgentPort, strServerIP, strServerPort);
		pItem->SetClickTextFont(2);
		pItem->OnNotify += MakeDelegate(this, &CMainDlg::ListItemNotify);
		m_pMapping_List->Add(pItem);
	}
}

bool CMainDlg::CheckAllInfo()
{
	if (!m_pEdit_server_port || !m_pEdit_agent_port || !m_pEdit_server_ip)
		return false;
	return m_pEdit_server_port->GetState() & m_pEdit_agent_port->GetState() & m_pEdit_server_ip->GetState();
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
		else if (pNotifyUI->pSender == m_pBtn_ADD)
		{
			OnAddClick();
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

void CMainDlg::UpDataConnectList(CControlUI* p_sender)
{
	m_pCur_mapping = static_cast<CMappingListItem*>(p_sender->GetInterface(DUI_CTR_MAPPINGLISTITEM));
	if (!m_pCur_mapping)
		return;
}

bool CMainDlg::ListItemNotify(void* p)
{
	TNotifyUI* pNotify = (TNotifyUI*)p;
	if (!pNotify)
		return false;
	if (pNotify->wParam != -1 && pNotify->sType == DUI_MSGTYPE_ITEMCLICK)
	{
		m_pConnect_List->SetVisible(true);
		if (m_pCur_mapping != pNotify->pSender)
		{
			UpDataConnectList(pNotify->pSender);
		}
		m_pBottom_hide->SetText(L"▼" );
	}
	return false;
}