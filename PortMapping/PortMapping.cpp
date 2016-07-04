// PortMapping.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "PortMapping.h"
#include "MainDlg.h"

inline void EnableMemLeakCheck()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    {
		CPaintManagerUI::SetInstance(hInstance);
		//设置资源路径
		CDuiString str = CPaintManagerUI::GetInstancePath();
		str += L"\\skin";
		CPaintManagerUI::SetResourcePath(str);
		//创建  显示对话框
		CMainDlg dlg;
		dlg.Create(nullptr, L"PortMapping", UI_WNDSTYLE_FRAME, UI_WNDSTYLE_EX_FRAME, 0, 0, 800, 572);
		dlg.CenterWindow();
		return dlg.ShowModal();
    }
}
