#pragma once
#define DUI_CTR_MYLISTITEM L"MyListItem"
#define DUI_CTR_MAPPINGLISTITEM L"MappingListItem"
#define DUI_CTR_CONNECTLISTITEM L"ConnectListItem"
#include "LibuvAdapter.h"
class CMyListItem : public DuiLib::CListContainerElementUI
{
public:
	CMyListItem();
	virtual ~CMyListItem();
public:
	//override
	virtual LPCTSTR GetClass() const override;
	virtual LPVOID GetInterface(LPCTSTR pstrName) override;

	virtual void DoEvent(DuiLib::TEventUI& event) override;
	virtual SIZE EstimateSize(SIZE szAvailable) override;
	virtual void DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl) override;

	virtual void DrawItemText(HDC hDC, const RECT& rcItem) override;
	virtual void DoInit() override;
	virtual UINT GetControlFlags() const override;
public:
	int AddText(const DuiLib::CDuiString& strData, bool bClick = false);//成功返回添加的序列从0开始，失败返回-1
	bool SetText(int nIndex, const DuiLib::CDuiString& strData);
	bool SetClick(int nIndex, bool bClick);
	bool DeleteText(int nIndex);
	bool InsetText(int nIndex, const DuiLib::CDuiString& strData, bool bClick = false);
	int ClearText();
	void SetClickTextColor(int nColor) { m_nClickCor = nColor; };
	void SetClickTextFont(int nFont) { m_nClictFont = nFont; };
	bool GetCheck();
	void SetCheck(bool b = true);
private:
	bool GetTextRect(int nIndex, RECT* prc, const RECT& rcitem);
private:
	DuiLib::CStdPtrArray	m_text_array;
	int						m_nClickCor;
	int						m_nClictFont;
	DuiLib::CCheckBoxUI*    m_pCheck_box;
};
//映射列表项
class CMappingListItem : public CMyListItem
{
public:
	CMappingListItem(MappingInfo* pInfo);
	virtual ~CMappingListItem();
public:
	virtual void DoInit() override;
	virtual LPCTSTR GetClass() const override;
	virtual LPVOID GetInterface(LPCTSTR pstrName) override;
	void InitStringList(const DuiLib::CDuiString& strAgentIP, const DuiLib::CDuiString& strAgentPort,
		const DuiLib::CDuiString& strServerIP, const DuiLib::CDuiString& strServerPort);
	bool Start();
	bool Stop();
	bool Delete();
	void Updata(bool bforce = false);		//bforce  是否强制刷新
	MappingInfo* GetInfo() { return m_pInfo; };
private:
	MappingInfo*		m_pInfo;
	CControlUI*			m_pCtrl;
};

//连接列表项
class CConnectListItem : public CMyListItem
{
public:
	CConnectListItem(ConnectInfo* pInfo);
	virtual ~CConnectListItem();
public:
	virtual void DoInit() override;
	virtual LPCTSTR GetClass() const override;
	virtual LPVOID GetInterface(LPCTSTR pstrName) override;
	
	bool Delete();
	void Updata(bool bforce = false);		//bforce  是否强制刷新
	ConnectInfo* GetInfo() { return m_pInfo; };
private:
	ConnectInfo*		m_pInfo;
};