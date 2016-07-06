#pragma once
#include "UIMenu.h"
using namespace DuiLib;
class CMainDlg : public WindowImplBase
{
public:
	CMainDlg();
	virtual ~CMainDlg();
	
protected:
	//实现基类纯虚接口
	virtual CDuiString GetSkinFolder();//设置资源文件夹
	virtual CDuiString GetSkinFile();//设置资源文件
	virtual LPCTSTR GetWindowClassName(void) const;//注册用的窗口类名
public:
	//重写基类虚函数
	virtual void InitWindow() override;//在OnCreate最后调用
	
	virtual LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, bool& bHandled) override;
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
private:
	bool RootNotify(void* p);
	bool ButtonNotify(void* pNotify);//按钮Notify消息
	bool ListNotify(void* pNotify);//列表Notify消息

	void OnMenuItemInit(CMenuElementUI* pMenuItem, LPARAM l_param);
	void OnMenuItemClick(LPCWSTR pName, LPARAM l_param);
	void Test();
	
private:
	CButtonUI*	m_pLeft_hide;		//左边隐藏 显示按钮
	CButtonUI*  m_pBottom_hide;		//下列表隐藏  显示按钮
	CButtonUI*  m_pMenu_hide;		//设置按钮
	CControlUI* m_pLeft_layout;		//左边添加信息的区域

	CListUI* m_pMapping_List;		//映射关系列表
	CListUI* m_pConnect_List;		//链接信息列表

};

