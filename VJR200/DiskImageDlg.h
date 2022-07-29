#ifndef __DISKIMAGEDLG_H__
#define __DISKIMAGEDLG_H__
#include "PC88Fdc.h"
#include "DiskImageCollection.h"

class DiskImageDlg
{
	// attribute
protected:
	// FDC
	static CPC88Fdc* m_pFdc;
	// disk image collection
	static CDiskImageCollection* m_pDiskImageCollection;
	// changed disk
	static bool m_bChangedDisk;
	// popupped list of combobox
	static bool m_bPopuppedComboList;

public:
	// set FDC
	//static void SetFdc(CPC88Fdc& fdc) {
	//	m_pFdc = &fdc;
	//}
	// set disk image collection
	//static void SetDiskImageCollection(CDiskImageCollection& dicDisks) {
	//	m_pDiskImageCollection = &dicDisks;
	//}

	// create & destroy
public:
	// standard constructor
	DiskImageDlg(HWND hwndParent);
	// destructor
	virtual ~DiskImageDlg();

	// implementation
protected:

	// dialog procedure
	static INT_PTR CALLBACK DlgProc(HWND hdlg, UINT nMessage, WPARAM wParam, LPARAM lParam);

	// operation
protected:
	// set children
	static void SetChildren(HWND hdlg, int nUpdateOfs = -1);
	// set remove erase button
	static void SetRemoveEraseButton(HWND hdlg);
	// on added disk image
	static  void OnImageAdd(HWND hdlg);
	// on removed disk image
	static void OnImageRemove(HWND hdlg);
	// on erase all disk image
	static void OnImageErase(HWND hdlg);
	// on changed disk selection
	static void OnDiskDriveChange(HWND hdlg, int nID);
	// on changed disk write-protect
	static void OnDiskDriveProtect(HWND hdlg, int nID);

public:
	// create modal dialog
	virtual int DoModal();
protected:
	static void ShowControl(HWND hwndControl, bool bShow);
	static void ClearListBox(HWND hwndListBox);
	static void ClearComboBox(HWND hwndComboBox);
	static void AddComboBoxItem(HWND hwndComboBox, const std::wstring & gstrItem);
	static int GetListBoxItemCount(HWND hwndListBox);
	static void DeleteListBoxItem(HWND hwndListBox, int nIndex);
	static int GetComboBoxItemCount(HWND hwndComboBox);
	static void DeleteComboBoxItem(HWND hwndComboBox, int nIndex);
	static void AddListBoxItem(HWND hwndListBox, const std::wstring & gstrItem);
	static void SetComboBoxSel(HWND hwndComboBox, int nSel);
	static void SetCheckButtonChecked(HWND hwndCheckButton, bool bCheck);
	static void EnableControl(HWND hwndControl, bool bEnable);
	static int GetListBoxSel(HWND hwndListBox, std::vector<int>* pvectSel);
	static int ShowMessageBox(HWND hwndParent, int nType, int nButtons, const std::wstring & gstrCaption, const std::wstring & gstrText);
	static void SetControlFocus(HWND hwndControl);
	static int GetComboBoxSel(HWND hwndComboBox);
	static bool IsCheckButtonChecked(HWND hwndCheckButton);
	static int ExecuteDialog(int nID, HWND hwndParent, DLGPROC pDlgProc, bool bModal);
	static void CreateFileNameList(const TCHAR * pfszFileName);

	// added by FIND
	static const int MAX_FILES_NUM = 4;
	static OPENFILENAME ofn;
	static BOOL OpenDialog(TCHAR* fileName, HWND hwnd);
	static OPENFILENAME m_ofn;
	static vector<std::wstring> m_fcntFileNames;
	static bool m_bReadOnly;
};

#endif