#pragma once
#define DUI_CTR_MYLISTITEM L"MyListItem"

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
	bool DeleteText(int nIndex);
	bool InsetText(int nIndex, const DuiLib::CDuiString& strData, bool bClick = false);
	int ClearText();
	void SetClickTextColor(int nColor) { m_nClickCor = nColor; };
	void SetClickTextFont(int nFont) { m_nClictFont = nFont; };
private:
	bool GetTextRect(int nIndex, RECT* prc, const RECT& rcitem);
private:
	DuiLib::CStdPtrArray	m_text_array;
	int						m_nClickCor;
	int						m_nClictFont;
	DuiLib::CCheckBoxUI*    m_pCheck_box;
};

