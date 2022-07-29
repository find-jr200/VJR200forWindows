// copyright-holders:Manuke, FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ディスクイメージダイアログに必要なダイアログプロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <Commdlg.h>
#include <commctrl.h>
#include "VJR200.h"
#include "JRSystem.h"
#include "DiskImageDlg.h"

extern JRSystem sys;

// FDC

CPC88Fdc* DiskImageDlg::m_pFdc;

// disk image collection

CDiskImageCollection* DiskImageDlg::m_pDiskImageCollection;

// changed disk

bool DiskImageDlg::m_bChangedDisk;

// popupped list of combobox

bool DiskImageDlg::m_bPopuppedComboList;

OPENFILENAME DiskImageDlg::m_ofn;
vector<std::wstring> DiskImageDlg::m_fcntFileNames;
bool DiskImageDlg::m_bReadOnly;
OPENFILENAME DiskImageDlg::ofn;


////////////////////////////////////////////////////////////
// create & destroy

// standard constructor

DiskImageDlg::DiskImageDlg(HWND hwndParent)
{
	m_pFdc = sys.pFddSystem->pFdc;
	m_pDiskImageCollection = &(sys.pFddSystem->GetDiskImageCollection());
	m_bChangedDisk = false;
	m_bPopuppedComboList = false;
	m_bReadOnly = false;
}

// destructor

DiskImageDlg::~DiskImageDlg() {
}

////////////////////////////////////////////////////////////
// implementation


// dialog procedure

INT_PTR CALLBACK DiskImageDlg::DlgProc(HWND hdlg, UINT nMessage, WPARAM wParam, LPARAM /*lParam*/)

{
	BOOL bResult = TRUE;
	switch (nMessage) {
	case WM_INITDIALOG:
	{ // dummy block
		//if (m_pFdc->GetDriveCount() <= 2) {
		//	RECT rectDrive1, rectDrive3;
		//	GetWindowRect(GetDlgItem(hdlg, IDC_DISK_DRIVE1), &rectDrive1);
		//	int nReduce = rectDrive3.top - rectDrive1.top;
		//	RECT rectReduce;
		//	GetWindowRect(hdlg, &rectReduce);
		//	SetWindowPos(
		//		hdlg, NULL,
		//		0,
		//		0,
		//		rectReduce.right - rectReduce.left,
		//		rectReduce.bottom - rectReduce.top - nReduce,
		//		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		//	HWND hstcGroup = GetDlgItem(hdlg, IDC_DISK_DRIVE_GROUP);
		//	GetWindowRect(hstcGroup, &rectReduce);
		//	SetWindowPos(
		//		hstcGroup, NULL,
		//		0,
		//		0,
		//		rectReduce.right - rectReduce.left,
		//		rectReduce.bottom - rectReduce.top - nReduce,
		//		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		//}
		for (int nDrive = 0; nDrive < 2; nDrive++) {
			SendDlgItemMessage(hdlg, IDC_DISK_DRIVE1 + nDrive,CB_SETEXTENDEDUI, TRUE, 0);
			if (nDrive >= m_pFdc->GetDriveCount()) {
				ShowControl(GetDlgItem(hdlg, IDC_DISK_DRIVE1 + nDrive),	false);
				ShowControl(GetDlgItem(hdlg, IDC_DISK_DRIVE1_LABEL + nDrive),false);
				ShowControl(GetDlgItem(hdlg, IDC_DISK_DRIVE1_PROTECT + nDrive),	false);
			}
		}
		// Centering(hdlg);
		SetChildren(hdlg);
		SetRemoveEraseButton(hdlg);
	}
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
		{
			if (m_pFdc->GetDiskImage(0) != nullptr)
				sys.pFddSystem->mountedFileName[0] = m_pFdc->GetDiskImage(0)->GetImageName();
			else
				sys.pFddSystem->mountedFileName[0] = L"";

			if (m_pFdc->GetDiskImage(1) != nullptr)
				sys.pFddSystem->mountedFileName[1] = m_pFdc->GetDiskImage(1)->GetImageName();
			else
				sys.pFddSystem->mountedFileName[1] = L"";

			m_pFdc->SetChangeStatus(true);

			EndDialog(hdlg, LOWORD(wParam));
			break;
		}
		case IDC_IMAGES:
			SetRemoveEraseButton(hdlg);
			break;
		case IDC_IMAGE_ADD:
			OnImageAdd(hdlg);
			break;
		case IDC_IMAGE_REMOVE:
			OnImageRemove(hdlg);
			break;
		case IDC_IMAGE_ERASE:
			OnImageErase(hdlg);
			break;
		case IDC_DISK_DRIVE1:
		case IDC_DISK_DRIVE2:
			switch (HIWORD(wParam)) {
			case CBN_DROPDOWN:
				m_bPopuppedComboList = true;
				m_bChangedDisk = false;
				break;
			case CBN_SELENDOK:
			case CBN_SELENDCANCEL:
				m_bPopuppedComboList = false;
				if (m_bChangedDisk) {
					m_bChangedDisk = false;
					OnDiskDriveChange(hdlg, LOWORD(wParam));
				}
				break;
			case CBN_SELCHANGE:
				if (m_bPopuppedComboList) {
					m_bChangedDisk = true;
				}
				else {
					OnDiskDriveChange(hdlg, LOWORD(wParam));
				}
				break;
			}
			break;
		case IDC_DISK_DRIVE1_PROTECT:
		case IDC_DISK_DRIVE2_PROTECT:
			if (HIWORD(wParam) == BN_CLICKED) {
				OnDiskDriveProtect(hdlg, LOWORD(wParam));
			}
			break;
		}
		break;
	default:
		bResult = FALSE;
		break;
	}
	return bResult;
}



////////////////////////////////////////////////////////////
// operation

// set children

void DiskImageDlg::SetChildren(HWND hdlg, int nUpdateOfs) {
	HWND hlistImages,
		ahcomboDiskDrive[CPC88Fdc::DRIVE_MAX],
		ahcheckDiskProtect[CPC88Fdc::DRIVE_MAX];
	if ((hlistImages = GetDlgItem(hdlg, IDC_IMAGES)) == NULL) {
		return;
	}
	int nDrive;
	for (nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
		ahcomboDiskDrive[nDrive] =
			GetDlgItem(hdlg, IDC_DISK_DRIVE1 + nDrive);
		if (ahcomboDiskDrive[nDrive] == NULL) {
			return;
		}
		ahcheckDiskProtect[nDrive] =
			GetDlgItem(hdlg, IDC_DISK_DRIVE1_PROTECT + nDrive);
		if (ahcheckDiskProtect[nDrive] == NULL) {
			return;
		}
	}
	std::wstring strTmp;
	CDiskImage* apDriveImage[CPC88Fdc::DRIVE_MAX];
	int anDriveIndex[CPC88Fdc::DRIVE_MAX];
	for (nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
		apDriveImage[nDrive] = m_pFdc->GetDiskImage(nDrive);
		anDriveIndex[nDrive] = 0;
	}
	int nFileIndex = 0, nDiskIndex = 1;
	CDiskImageCollection::iterator itImageFile =
		m_pDiskImageCollection->begin();
	if (nUpdateOfs < 0) {
		ClearListBox(hlistImages);
		for (nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
			ClearComboBox(ahcomboDiskDrive[nDrive]);
			AddComboBoxItem(ahcomboDiskDrive[nDrive],_T("<none>"));
		}
	}
	else {
		int nDeleteCount;
		for (
			nDeleteCount =
			GetListBoxItemCount(hlistImages) - nUpdateOfs;
			nDeleteCount > 0;
			nDeleteCount--)
		{
			DeleteListBoxItem(hlistImages, nUpdateOfs);
		}
		for (
			;
			itImageFile != m_pDiskImageCollection->end();
			itImageFile++)
		{
			if (nFileIndex >= nUpdateOfs) {
				break;
			}
			for (
				CDiskImageFile::iterator itImage = (*itImageFile).begin();
				itImage != (*itImageFile).end();
				itImage++)
			{
				for (nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
					if ((apDriveImage[nDrive] != NULL) &&
						(*apDriveImage[nDrive] == *itImage))
					{
						anDriveIndex[nDrive] = nDiskIndex;
					}
				}
				nDiskIndex++;
			}
			nFileIndex++;
		}
		for (nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
			for (
				nDeleteCount =
				GetComboBoxItemCount(ahcomboDiskDrive[nDrive]) - nDiskIndex;
				nDeleteCount > 0;
				nDeleteCount--)
			{
				DeleteComboBoxItem(ahcomboDiskDrive[nDrive], nDiskIndex);
			}
		}
	}

	// D88ファイル設定
	for (; itImageFile != m_pDiskImageCollection->end(); itImageFile++) {
		std::wstring fstrFileName = (*itImageFile).GetFileName(),gstrFileName = fstrFileName;
		const TCHAR* pgszFileName = gstrFileName.c_str();


		TCHAR fszName[_MAX_FNAME], fszExt[_MAX_EXT];
		_wsplitpath(pgszFileName, NULL, NULL, fszName, fszExt);
		TCHAR tmp[255];
		wsprintf(tmp, _T("%d:%s%s(%d)%s"), nFileIndex + 1, fszName, fszExt, (*itImageFile).size(), (*itImageFile).IsReadOnly() ? "<R>" : "");
		strTmp = tmp;
		AddListBoxItem(hlistImages, strTmp);

		// Diskイメージ設定
		for (CDiskImageFile::iterator itImage = (*itImageFile).begin(); itImage != (*itImageFile).end(); itImage++) {
			std::wstring fstrImageName = (*itImage).GetImageName(),	gstrImageName = fstrImageName;
			wsprintf(tmp, _T("%d:%s"), nFileIndex + 1, gstrImageName.c_str());
			strTmp = tmp;
			for (nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
				AddComboBoxItem(ahcomboDiskDrive[nDrive], strTmp);
				if ((apDriveImage[nDrive] != NULL) &&
					(*apDriveImage[nDrive] == *itImage)) {
					anDriveIndex[nDrive] = nDiskIndex;
				}
			}
			nDiskIndex++;
		}
		nFileIndex++;
	}

	// コンボボックスセレクト
	for (nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
		SetComboBoxSel(ahcomboDiskDrive[nDrive], anDriveIndex[nDrive]);
		bool bEnable = false, bCheck = false;
		if (anDriveIndex[nDrive] > 0) {
			bEnable = true;
			if (apDriveImage[nDrive]->IsWriteProtected()) {
				bCheck = true;
			}
		}
		SetCheckButtonChecked(ahcheckDiskProtect[nDrive], bCheck);
		EnableControl(ahcheckDiskProtect[nDrive], bEnable);
	}
}

// set remove erase button

void DiskImageDlg::SetRemoveEraseButton(HWND hdlg) {
	HWND hbuttonRemove = GetDlgItem(hdlg, IDC_IMAGE_REMOVE),
		hbuttonErase = GetDlgItem(hdlg, IDC_IMAGE_ERASE);
	if ((hbuttonRemove != NULL) && (hbuttonErase != NULL)) {
		int nCount = m_pDiskImageCollection->size(),
			nSelCount = GetListBoxSel(GetDlgItem(hdlg, IDC_IMAGES), NULL);
		EnableControl(hbuttonRemove, nSelCount > 0);
		EnableControl(hbuttonErase, nCount > 0);
	}
}

// on added disk image

void DiskImageDlg::OnImageAdd(HWND hdlg) {
	TCHAR fileName[MAX_PATH * MAX_FILES_NUM];
	fileName[0] = _T('\0');
	
	if (OpenDialog(fileName, g_hMainWnd)) {
		int nUpdateOfs = m_pDiskImageCollection->size(),
			nOldDiskCount = m_pDiskImageCollection->GetDiskImageCount();
		bool bResult = true;

		m_fcntFileNames.clear();
		CreateFileNameList(fileName);

		int loopCount = (int)(m_fcntFileNames.size() > MAX_FILES_NUM ? MAX_FILES_NUM : m_fcntFileNames.size());
		for (int nIndex = 0; nIndex < loopCount; nIndex++) {
			if (!m_pDiskImageCollection->AddDiskImageFile(m_fcntFileNames.at(nIndex), false /*dlgFile.IsReadOnly()*/))
			{
				bResult = false;
				break;
			}
		}

		if (!bResult) {
			ShowMessageBox(hdlg, MB_ICONEXCLAMATION, MB_OK, _T("Disk Image"), _T("An error occurred while reading."));
		}
		else {
			int nDiskIndex = nOldDiskCount,
				nDiskIndexMax = m_pDiskImageCollection->GetDiskImageCount();
			for (int nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
				if (nDiskIndex >= nDiskIndexMax) {
					break;
				}
				if (m_pFdc->GetDiskImage(nDrive) == NULL) {
					m_pFdc->SetDiskImage(nDrive, m_pDiskImageCollection->GetDiskImage(nDiskIndex));
					nDiskIndex++;
				}
			}
		}
		SetChildren(hdlg, nUpdateOfs);
		SetRemoveEraseButton(hdlg);
	}
}

// on removed disk image

void DiskImageDlg::OnImageRemove(HWND hdlg) {
	int nIndexMax = m_pDiskImageCollection->size();
	std::vector<int> vectRemove;
	GetListBoxSel(GetDlgItem(hdlg, IDC_IMAGES), &vectRemove);
	if (vectRemove.size() <= 0) {
		return;
	}
	CDiskImageCollection::iterator itImageFile =
		m_pDiskImageCollection->begin();
	int nUpdateOfs = -1, nSelIndex = 0;
	for (int nIndex = 0; nIndex < nIndexMax; nIndex++) {
		if (vectRemove[nSelIndex] == nIndex) {
			nSelIndex++;
			if (nUpdateOfs < 0) {
				nUpdateOfs = nIndex;
			}
			for (int nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
				CDiskImage* pDriveImage =
					m_pFdc->GetDiskImage(nDrive);
				if ((pDriveImage != NULL) &&
					(pDriveImage->GetFileID() ==
					(*itImageFile).GetFileID()))
				{
					m_pFdc->SetDiskImage(nDrive, NULL);
				}
			}
			itImageFile =
				m_pDiskImageCollection->erase(itImageFile);
			if (nSelIndex >= (int)vectRemove.size()) {
				break;
			}
		}
		else {
			itImageFile++;
		}
	}
	SetChildren(hdlg, nUpdateOfs);
	SetRemoveEraseButton(hdlg);
	SetControlFocus(GetDlgItem(hdlg, IDOK));
}

// on erase all disk image

void DiskImageDlg::OnImageErase(HWND hdlg) {
	int nCount = m_pDiskImageCollection->size();
	if (nCount <= 0) {
		return;
	}
	for (int nDrive = 0; nDrive < m_pFdc->GetDriveCount(); nDrive++) {
		m_pFdc->SetDiskImage(nDrive, NULL);
	}
	m_pDiskImageCollection->clear();
	SetChildren(hdlg);
	SetRemoveEraseButton(hdlg);
	SetControlFocus(GetDlgItem(hdlg, IDOK));
}

// on changed disk selection

void DiskImageDlg::OnDiskDriveChange(HWND hdlg, int nID) {
	int nDrive = nID - IDC_DISK_DRIVE1;
	int nDiskIndex = GetComboBoxSel(GetDlgItem(hdlg, nID));
	HWND hcheckDiskProtect = GetDlgItem(
		hdlg, IDC_DISK_DRIVE1_PROTECT + nDrive);
	if (nDiskIndex <= 0) {
		m_pFdc->SetDiskImage(nDrive, NULL);
		if (nDiskIndex < 0) {
			SetComboBoxSel(GetDlgItem(hdlg, nID), 0);
		}
		SetCheckButtonChecked(hcheckDiskProtect, false);
		EnableControl(hcheckDiskProtect, false);
	}
	else {
		SetCheckButtonChecked(hcheckDiskProtect,m_pDiskImageCollection->GetDiskImage(nDiskIndex - 1)->IsWriteProtected());
		EnableControl(hcheckDiskProtect, true);
		for (int nDrive2 = 0; nDrive2 < m_pFdc->GetDriveCount(); nDrive2++) {
			if (nDrive2 == nDrive) {
				continue;
			}
			int nDiskIndex2 = m_pDiskImageCollection->GetDiskImageIndex(
				m_pFdc->GetDiskImage(nDrive2));
			if (nDiskIndex2 >= 0) {
				nDiskIndex2++;
			}
			if (nDiskIndex2 == nDiskIndex) {
				m_pFdc->SetDiskImage(nDrive2, NULL);
				SetComboBoxSel(GetDlgItem(hdlg, IDC_DISK_DRIVE1 + nDrive2), 0);
				hcheckDiskProtect = GetDlgItem(
					hdlg, IDC_DISK_DRIVE1_PROTECT + nDrive2);
				SetCheckButtonChecked(
					hcheckDiskProtect, false);
				EnableControl(
					hcheckDiskProtect, false);
			}
		}
		m_pFdc->SetDiskImage(
			nDrive,
			m_pDiskImageCollection->GetDiskImage(nDiskIndex - 1));
	}
}

// on changed disk write-protect

void DiskImageDlg::OnDiskDriveProtect(HWND hdlg, int nID) {
	int nDrive = nID - IDC_DISK_DRIVE1_PROTECT;
	bool bProtect = IsCheckButtonChecked(GetDlgItem(hdlg, nID));
	CDiskImage* pDriveImage = m_pFdc->GetDiskImage(nDrive);
	if (pDriveImage == NULL) {
		return;
	}
	pDriveImage->SetWriteProtect(bProtect);
}

// create modal dialog

int DiskImageDlg::DoModal() {
	return ExecuteDialog(IDD_DISKIMAGE,	g_hMainWnd, DlgProc, true);
}





void DiskImageDlg::ShowControl(HWND hwndControl, bool bShow)
{

	if (hwndControl == NULL) {
		return;
	}
	ShowWindow(hwndControl, bShow ? SW_SHOWNOACTIVATE : SW_HIDE);

}

void DiskImageDlg::ClearListBox(HWND hwndListBox)
{
	if (hwndListBox == NULL) {
		return;
	}
	SendMessage(hwndListBox, LB_RESETCONTENT, 0, 0);
}

void DiskImageDlg::ClearComboBox(HWND hwndComboBox)
{
	if (hwndComboBox == NULL) {
		return;
	}
	SendMessage(hwndComboBox, CB_RESETCONTENT, 0, 0);

}

void DiskImageDlg::AddComboBoxItem(HWND hwndComboBox, const std::wstring& gstrItem)
{
	if (hwndComboBox == NULL) {
		return;
	}
	SendMessage(hwndComboBox,CB_ADDSTRING, 0, (LPARAM)gstrItem.c_str());

}


int DiskImageDlg::GetListBoxItemCount(HWND hwndListBox)
{
	if (hwndListBox == NULL) {
		return 0;
	}
	return (int)SendMessage(hwndListBox, LB_GETCOUNT, 0, 0);
}


void DiskImageDlg::DeleteListBoxItem(HWND hwndListBox, int nIndex)
{
	if (hwndListBox == NULL) {
		return;
	}
	SendMessage(hwndListBox,LB_DELETESTRING, nIndex, 0);
}


int DiskImageDlg::GetComboBoxItemCount(HWND hwndComboBox)
{
	if (hwndComboBox == NULL) {
		return 0;
	}
	return (int)SendMessage(hwndComboBox, CB_GETCOUNT, 0, 0);
}


void DiskImageDlg::DeleteComboBoxItem(HWND hwndComboBox, int nIndex)
{
	if (hwndComboBox == NULL) {
		return;
	}
	SendMessage(hwndComboBox, CB_DELETESTRING, nIndex, 0);
}



void DiskImageDlg::AddListBoxItem(HWND hwndListBox,	const std::wstring& gstrItem)
{
	if (hwndListBox == NULL) {
		return;
	}
	SendMessage(hwndListBox,LB_ADDSTRING, 0, (LPARAM)gstrItem.c_str());
}


void DiskImageDlg::SetComboBoxSel(HWND hwndComboBox, int nSel)
{
	if (hwndComboBox == NULL) {
		return;
	}
	SendMessage(hwndComboBox, CB_SETCURSEL, nSel, 0);
}


void DiskImageDlg::SetCheckButtonChecked(HWND hwndCheckButton,	bool bCheck)
{
	if (hwndCheckButton == NULL) {
		return;
	}
	SendMessage(hwndCheckButton,BM_SETCHECK, bCheck ? BST_CHECKED : BST_UNCHECKED, 0);
}


void DiskImageDlg::EnableControl(HWND hwndControl,	bool bEnable)
{
	if (hwndControl == NULL) {
		return;
	}
	EnableWindow(hwndControl, bEnable ? TRUE : FALSE);
}


int DiskImageDlg::GetListBoxSel(HWND hwndListBox, std::vector<int>* pvectSel)
{
	if (hwndListBox == NULL) {
		if (pvectSel != NULL) {
			pvectSel->clear();
		}
		return 0;
	}
	int nSelCount = (int)SendMessage(hwndListBox, LB_GETSELCOUNT, 0, 0);
	if (nSelCount == LB_ERR) {
		nSelCount = 0;
	}
	if (pvectSel != NULL) {
		pvectSel->clear();
		pvectSel->reserve(nSelCount);
		for (
			int nIndex = 0,
			nCount = (int)SendMessage(hwndListBox, LB_GETCOUNT, 0, 0);
			nIndex < nCount;
			nIndex++)
		{
			if (SendMessage(hwndListBox, LB_GETSEL, nIndex, 0) > 0) {
				pvectSel->push_back(nIndex);
			}
		}
	}
	return nSelCount;
}



int DiskImageDlg::ShowMessageBox(HWND hwndParent,int nType, int nButtons,const std::wstring& gstrCaption,	const std::wstring& gstrText)
{
	return MessageBox(hwndParent,gstrText.c_str(),gstrCaption.c_str(),nType | nButtons);
}


void DiskImageDlg::SetControlFocus(HWND hwndControl)
{
	if (hwndControl == NULL) {
		return;
	}
	SetFocus(hwndControl);
}


int DiskImageDlg::GetComboBoxSel(HWND hwndComboBox)
{
	if (hwndComboBox == NULL) {
		return -1;
	}
	return (int)SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0);
}


bool DiskImageDlg::IsCheckButtonChecked(HWND hwndCheckButton)
{
	if (hwndCheckButton == NULL) {
		return false;
	}
	return SendMessage(
		hwndCheckButton,
		BM_GETCHECK, 0, 0) == BST_CHECKED;

}


int DiskImageDlg::ExecuteDialog(int nID, HWND hwndParent,DLGPROC pDlgProc,bool bModal)
{
	int nResult;
	if (bModal) {
		nResult = (int)DialogBox(g_hMod, MAKEINTRESOURCE(nID),hwndParent,(DLGPROC)pDlgProc);
	}
	else {
		nResult = 0;
		HWND hdlg = CreateDialog(
			g_hInst, MAKEINTRESOURCE(nID),hwndParent,pDlgProc);
		if (hdlg != NULL) {
			ShowWindow(hdlg, SW_SHOWNORMAL);
			nResult = 1;
		}
	}
	return nResult;

}





BOOL DiskImageDlg::OpenDialog(TCHAR* fileName, HWND hwnd)
{
	memset(&m_ofn, 0x00, sizeof(m_ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = _T("D20 file(*.d20)\0*.d20\0");
	ofn.lpstrDefExt = _T("D20");
	ofn.lpstrTitle = _T("Specify D20 File");
	ofn.nMaxCustFilter = 256;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH * MAX_FILES_NUM;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

	return GetOpenFileName(&ofn);
}



// create file name list

void DiskImageDlg::CreateFileNameList(const TCHAR* pfszFileName) {
	int nLength = (int)_tcslen(pfszFileName);
	if (nLength > 0) {
		const TCHAR* pfszFileName2 = pfszFileName + nLength + 1;
		if (*pfszFileName2 != _T('\0')) {
			for ( ;*pfszFileName2 != _T('\0'); pfszFileName2 += _tcslen(pfszFileName2) + 1)
			{
				TCHAR* pfszFileName3 = new TCHAR[_MAX_PATH];
				_tmakepath(pfszFileName3, NULL, pfszFileName, pfszFileName2, NULL);
				m_fcntFileNames.push_back(pfszFileName3);
				delete[] pfszFileName3;
			}
		}
		else {
			m_fcntFileNames.push_back(pfszFileName);
		}
	}
}
