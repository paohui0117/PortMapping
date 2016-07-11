#pragma once
#define  DUI_CTR_EDITEX                            (_T("EditEx"))
struct CheckInfo
{
	DuiLib::CDuiString	m_content;			//待检测的内容
	DuiLib::CDuiString	m_waring_info;		//错误提示内容
};
class CEditUIEx : public DuiLib::CEditUI
{
	friend class CEditWndEx;
public:
	CEditUIEx();
	virtual ~CEditUIEx();
public:
	virtual LPCTSTR GetClass() const override;
	virtual LPVOID GetInterface(LPCTSTR pstrName) override;
	virtual void PaintBorder(HDC hDC) override;
	virtual void DoEvent(DuiLib::TEventUI& event) override;
	virtual void SetText(LPCTSTR pstrText) override;
	virtual HWND GetNativeEditHWND() const override;
	virtual void SetPos(RECT rc, bool bNeedInvalidate = true) override;
	virtual void Move(SIZE szOffset, bool bNeedInvalidate = true) override;
	virtual void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue) override;
	virtual void SetInternVisible(bool bVisible = true) override;
public:
	void SetShowWaring(bool bWaring = true) { m_bShowWaring = bWaring; };
	static void SetWaringColor(DWORD nCol) { CEditUIEx::m_nWaringCol = nCol; };
	bool GetState() { return m_bOK; };
private:
	bool CheckContent();
public:
	static DWORD 	m_nWaringCol;	//错误外边框颜色
	DuiLib::CEventSource OnCheck;	//检测的委托
	
private:
	CEditWndEx*		m_pWindowEx;
	bool			m_bShowWaring;	//是否显示错误框
	bool			m_bOK;			//内容检测是否通过
	CheckInfo		m_check_info;
};

