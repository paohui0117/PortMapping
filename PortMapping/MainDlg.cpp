#include "stdafx.h"
#include "MainDlg.h"
#include "resource.h"

CMainDlg::CMainDlg()
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

