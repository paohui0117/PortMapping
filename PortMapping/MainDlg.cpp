#include "stdafx.h"
#include "MainDlg.h"
#include "UIMenu.h"
#include "EditUIEx.h"
#define MAIN_UPDATA_TIMER 1234

#define USER_CONNECT_MSG	(WM_USER+100)
#define USER_MAPPING_MSG	(WM_USER + 101)
#define USER_ALLCONNECT_MSG (WM_USER + 102)
CMainDlg::CMainDlg() :
	m_pLeft_hide(nullptr), m_pBottom_hide(nullptr), m_pLeft_layout(nullptr),
	m_pMapping_List(nullptr), m_pConnect_List(nullptr), m_pMenu_hide(nullptr),
	m_pEdit_agent_port(nullptr), m_pEdit_server_port(nullptr), m_pEdit_server_ip(nullptr),
	m_pCmb_protocol(nullptr), m_pBtn_ADD(nullptr), m_pCur_mapping(nullptr),
	m_pCmb_agent_ip(nullptr), m_pCheck_Mapping(nullptr), m_pCheck_Connect(nullptr)
{
	m_pLibuv = new CLibuvAdapter;
	m_pLibuv->AddNotify(this);
	m_pregex_IP = new wregex(L"^((25[0-5]|2[0-4]\\d|[01]?\\d\\d?)($|(?!\\.$)\\.)){4}$");
	m_u_cur.m_pMapping = nullptr;
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
		m_pMapping_List->GetList()->SetContextMenuUsed(false);
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

	pCur = m_PaintManager.FindSubControlByName(nullptr, L"check_mapping");
	if (pCur->GetInterface(DUI_CTR_CHECKBOX))
	{
		m_pCheck_Mapping = static_cast<CCheckBoxUI*>(pCur);
		m_pCheck_Mapping->OnNotify += MakeDelegate(this, &CMainDlg::CheckNotify);
	}
	pCur = m_PaintManager.FindSubControlByName(nullptr, L"check_connect");
	if (pCur->GetInterface(DUI_CTR_CHECKBOX))
	{
		m_pCheck_Connect = static_cast<CCheckBoxUI*>(pCur);
		m_pCheck_Connect->OnNotify += MakeDelegate(this, &CMainDlg::CheckNotify);
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
	for (int i = 0; i < pInfo->m_content.GetLength(); i++)
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
	for (int i = 0; i < m_pMapping_List->GetCount(); i++)
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
	for (int i = 0; i < m_pConnect_List->GetCount(); i++)
	{
		pCtrl = m_pConnect_List->GetItemAt(i);
		if (!pCtrl)
			continue;
		pConnectItem = static_cast<CConnectListItem *>(pCtrl->GetInterface(DUI_CTR_CONNECTLISTITEM));
		if (pConnectItem)
			pConnectItem->Updata();
	}
}

bool CMainDlg::CheckNotify(void* p)
{
	TNotifyUI* pNotify = (TNotifyUI*)p;
	if (!pNotify)
		return false;
	if (pNotify->sType == DUI_MSGTYPE_SELECTCHANGED)
	{
		CMyListItem* pItem = nullptr;
		if (pNotify->pSender == m_pCheck_Mapping)
		{
			for (size_t i = 0; i < m_pMapping_List->GetCount(); i++)
			{
				pItem = (CMyListItem*)m_pMapping_List->GetItemAt(i);
				pItem->SetCheck(m_pCheck_Mapping->GetCheck());
			}
		}
		else if (pNotify->pSender == m_pCheck_Connect)
		{
			for (size_t i = 0; i < m_pConnect_List->GetCount(); i++)
			{
				pItem = (CMyListItem*)m_pConnect_List->GetItemAt(i);
				pItem->SetCheck(m_pCheck_Connect->GetCheck());
			}
		}
	}
	return false;
}

LRESULT CMainDlg::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (uMsg == WM_USER_MENUITEM_CLICK)
	{
		m_PaintManager.SendNotify(m_PaintManager.GetRoot(), DUI_MSGTYPE_MENUITEM_CLICK, wParam, lParam);
		bHandled = true;
		return S_OK;
	}
	else if(uMsg == USER_CONNECT_MSG)
	{
		DealWithConnectMsg(wParam, (ConnectInfo*)lParam);
		bHandled = true;
		return S_OK;
	}
	else if (uMsg == USER_MAPPING_MSG)
	{
		DealWithMappingMsg(wParam, (MappingInfo*)lParam);
		bHandled = true;
		return S_OK;
	}
	else if (uMsg == USER_ALLCONNECT_MSG)
	{
		ConnectInfo** pInfo = (ConnectInfo**)wParam;
		size_t size = lParam;
		bHandled = true;
		if (!pInfo || size == 0)
			return S_OK;
		if (!m_pCur_mapping || (*pInfo)->pMapping != m_pCur_mapping->GetInfo())
			return S_OK;
		for (size_t i = 0; i < size; i++)
		{
			if ((*pInfo)->bInMap != IN_MAP_SUCC)//只显示成功的链接
				continue;
			CConnectListItem* pitem = new CConnectListItem(*pInfo);
			m_pConnect_List->Add(pitem);
			++pInfo;
		}
		m_pConnect_List->SetVisible();
		return S_OK;
	}
	bHandled = false;
	return 0;
}

void CMainDlg::NotifyConnectMessage(UINT nType, ConnectInfo* pInfo)
{
	//通过sendmessage达到同步的作用
	::SendMessage(m_hWnd, USER_CONNECT_MSG, nType, (LPARAM)pInfo);
}

void CMainDlg::NotifyMappingMessage(UINT nType, MappingInfo* pInfo)
{
	::SendMessage(m_hWnd, USER_MAPPING_MSG, nType, (LPARAM)pInfo);
}

void CMainDlg::NotifyGetAllConnectByMapping(ConnectInfo** pInfo, size_t size)
{
	::SendMessage(m_hWnd, USER_ALLCONNECT_MSG, (WPARAM)pInfo, (LPARAM)size);
}

void CMainDlg::OnMenuItemInit(CMenuElementUI* pMenuItem, LPARAM l_param)
{
	if (!pMenuItem)
		return;
	if (!m_u_cur.m_pMapping)//没能记录是哪个列表项弹出的
		return;
	CDuiString pStrName = pMenuItem->GetName();
	if (wcscmp(pStrName, L"start") == 0)//开始
	{
		pMenuItem->SetEnabled(!(m_u_cur.m_pMapping->GetInfo()->nState & (MAPPING_START | MAPPING_DELETING)));
	}
	else if (wcscmp(pStrName, L"stop") == 0)
	{
		pMenuItem->SetEnabled(m_u_cur.m_pMapping->GetInfo()->nState & (MAPPING_START | MAPPING_DELETING));
	}
	else if (wcscmp(pStrName, L"delete") == 0)
	{
		pMenuItem->SetEnabled(!(m_u_cur.m_pMapping->GetInfo()->nState & MAPPING_DELETING));
	}
	else if (wcscmp(pStrName, L"delete2") == 0)
	{
		pMenuItem->SetEnabled(!(m_u_cur.m_pConnect->GetInfo()->bDeleting));
	}
	else if (wcscmp(pStrName, L"start_sel") == 0)
	{
		pMenuItem->SetEnabled(AnlySelect(m_pMapping_List));
	}
	else if (wcscmp(pStrName, L"stop_sel") == 0)
	{
		pMenuItem->SetEnabled(AnlySelect(m_pMapping_List));
	}
	else if (wcscmp(pStrName, L"delete_sel") == 0)
	{
		pMenuItem->SetEnabled(AnlySelect(m_pMapping_List));
	}
	else if (wcscmp(pStrName, L"delete_sel2") == 0)
	{
		pMenuItem->SetEnabled(AnlySelect(m_pConnect_List));
	}
}

void CMainDlg::OnMenuItemClick(LPCWSTR pName, LPARAM l_param)
{
	if (!pName)
		return;
	if (!m_u_cur.m_pMapping)//没能记录是哪个列表项弹出的
		return;
	if (wcscmp(pName, L"start") == 0)//开始
	{
		StartMapping();
	}
	else if (wcscmp(pName, L"stop") == 0)
	{
		StopMapping();
	}
	else if (wcscmp(pName, L"stop") == 0)
	{
		StopMapping();
	}
	else if (wcscmp(pName, L"delete") == 0)
	{
		DeleteMapping();
	}
	else if (wcscmp(pName, L"delete2") == 0)
	{
		DeleteConnect();
	}
	else if (wcscmp(pName, L"start_sel") == 0)
	{
		StartMapping(true);
	}
	else if (wcscmp(pName, L"stop_sel") == 0)
	{
		StopMapping(true);
	}
	else if (wcscmp(pName, L"delete_sel") == 0)
	{
		DeleteMapping(true);
	}
	else if (wcscmp(pName, L"delete_sel2") == 0)
	{
		DeleteConnect(true);
	}
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

void CMainDlg::DealWithConnectMsg(WPARAM w_param, ConnectInfo* connect_info)
{
	if (w_param == MSG_ADD_CONNECT || w_param == MSG_CONNECT_STATE_CHANGE)
	{
		if (m_pCur_mapping && connect_info->pMapping == m_pCur_mapping->GetInfo() && 
			connect_info->bInMap == IN_MAP_SUCC)
		{
			CConnectListItem* pItem = new CConnectListItem(connect_info);
			m_pConnect_List->Add(pItem);
			m_pConnect_List->SetVisible();
		}
	}
	else if (w_param == MSG_DELETE_CONNECT)
	{
		if (m_pCur_mapping && connect_info->pMapping == m_pCur_mapping->GetInfo())
		{
			m_pConnect_List->Remove((CControlUI*)connect_info->pUserData);
			connect_info->pUserData = nullptr;
		}
	}
}

void CMainDlg::DealWithMappingMsg(WPARAM w_param, MappingInfo* mapping_info)
{
	if (!mapping_info)
		return;
	if (w_param == MSG_CLEAR_CONNECT || w_param == MSG_LISTEN_FAIL || w_param == MSG_MAPPING_STOP)
	{
		if (m_pCur_mapping && mapping_info == m_pCur_mapping->GetInfo())
		{
			m_pConnect_List->RemoveAll();
		}
		if (w_param == MSG_LISTEN_FAIL || w_param == MSG_MAPPING_STOP)
		{
			CMappingListItem* pItem = (CMappingListItem*)mapping_info->pUserData;
			if (!pItem)
				return;
			pItem->Updata(true);
		}
	}
	else if (w_param == MSG_REMOVE_MAPPING)
	{
		if (m_pCur_mapping && mapping_info == m_pCur_mapping->GetInfo())
		{
			m_pConnect_List->RemoveAll();
			m_pCur_mapping = nullptr;
		}
		if (mapping_info->pUserData)
		{
			m_pMapping_List->Remove((CControlUI*)mapping_info->pUserData);
			mapping_info->pUserData = nullptr;
		}
	}
}

void CMainDlg::StartMapping(bool bAll)
{
	MappingOperate(&CLibuvAdapter::StartMapping, bAll);
}

void CMainDlg::StopMapping(bool bAllSelect)
{
	MappingOperate(&CLibuvAdapter::StopMapping, bAllSelect);
}

void CMainDlg::DeleteMapping(bool bAllSelect)
{
	MappingOperate(&CLibuvAdapter::RemoveMapping, bAllSelect);
}

bool CMainDlg::MappingOperate(Func fun, bool bAllSelect)
{
	if (!bAllSelect)
	{
		if (!m_u_cur.m_pMapping)
			return false;
		return (m_pLibuv->*fun)(m_u_cur.m_pMapping->GetInfo());
	}
	
	CMappingListItem* pItem = nullptr;
	for (int i = 0; i < m_pMapping_List->GetCount(); i++)
	{
		pItem = (CMappingListItem*)m_pMapping_List->GetItemAt(i);
		if (!pItem->GetCheck())
			continue;
		(m_pLibuv->*fun)(pItem->GetInfo());
	}
	return true;
}

void CMainDlg::DeleteConnect(bool bAllSelect)
{
	if (!bAllSelect)
	{
		if (!m_u_cur.m_pConnect)
			return;
		m_pLibuv->RemoveConnect(m_u_cur.m_pConnect->GetInfo());
		return;
	}
	CConnectListItem* pItem = nullptr;
	for (int i = 0; i < m_pConnect_List->GetCount(); i++)
	{
		pItem = (CConnectListItem*)m_pConnect_List->GetItemAt(i);
		if (!pItem->GetCheck())
			continue;
		m_pLibuv->RemoveConnect(pItem->GetInfo());
	}
}

bool CMainDlg::AnlySelect(CListUI* pList)
{
	if (!pList)
		return false;
	CMyListItem* pItem = nullptr;
	for (size_t i = 0; i < pList->GetCount(); i++)
	{
		pItem = (CMyListItem*)pList->GetItemAt(i);
		if (pItem->GetCheck())
			return true;
	}
	return false;
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
		if (pNotifyUI->wParam < 1)
			return true;
		CListUI* pList = (CListUI*)pNotifyUI->pSender;
		RECT rcHead = pList->GetHeader()->GetPos();
		if (PtInRect(&rcHead, pNotifyUI->ptMouse))
			return true;
		POINT pt = pNotifyUI->ptMouse;
		ClientToScreen(m_hWnd, &pt);
		if (pList == m_pMapping_List)
		{
			m_u_cur.m_pMapping = (CMappingListItem*)m_pMapping_List->GetItemAt(pNotifyUI->wParam - 1);
			STRINGorID xml(L"mappingmenu.xml");
			ShowMenu(&m_PaintManager, xml, pt);
		}
		else if (pList == m_pConnect_List)
		{
			m_u_cur.m_pConnect = (CConnectListItem*)m_pConnect_List->GetItemAt(pNotifyUI->wParam - 1);
			STRINGorID xml(L"connectmenu.xml");
			ShowMenu(&m_PaintManager, xml, pt);
		}
	}
	return true;
}

void CMainDlg::UpDataConnectList(CControlUI* p_sender)
{
	m_pCur_mapping = static_cast<CMappingListItem*>(p_sender->GetInterface(DUI_CTR_MAPPINGLISTITEM));
	//没有开始映射，直接返回
	if (!m_pCur_mapping || !(m_pCur_mapping->GetInfo()->nState & MAPPING_START))
		return;
	m_pConnect_List->RemoveAll();
	m_pLibuv->GetAllConnect(m_pCur_mapping->GetInfo());//获取所有的连接
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