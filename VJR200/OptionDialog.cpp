// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// オプションダイアログに必要なダイアログプロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <Commdlg.h>
#include <commctrl.h>
#include "VJR200.h"
#include "JRSystem.h"
#include "OptionDialog.h"

extern JRSystem sys;
static int keyDefPressed = 0;
static HHOOK hHook = NULL;
static HWND hDlg5 = 0;

static JOYINFOEX info;
static int buttonDefPressed = 0;
static HWND hTab;
static HWND hTab1, hTab2, hTab3, hTab4, hTab5;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// エミュレータ設定タブ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK Tab1Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR str[20];
	int pos3 = g_quickTypeS;
	HWND hBar3 = GetDlgItem(hDlg, IDC_OPTION_QUICKTYPE);

	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG: 
	{
		HWND hCmbo = GetDlgItem(hDlg, IDC_OPTOIN_LANGUAGE);
		SendMessage(hCmbo, CB_INSERTSTRING, 0, (LPARAM)_T("English"));
		SendMessage(hCmbo, CB_INSERTSTRING, 0, (LPARAM)_T("日本語"));
		SendMessage(hCmbo, CB_INSERTSTRING, 0, (LPARAM)_T("Auto"));
		SendMessage(hCmbo, CB_SETCURSEL, (WPARAM)g_language, 0);

		if (g_keyboard == 0) { // JP
			CheckDlgButton(hDlg, IDC_OPTION_KBDJP, 1);
			CheckDlgButton(hDlg, IDC_OPTION_KBDEN, 0);
		}
		else if (g_keyboard == 1) { //EN
			CheckDlgButton(hDlg, IDC_OPTION_KBDJP, 0);
			CheckDlgButton(hDlg, IDC_OPTION_KBDEN, 1);
		}

		_itot(g_refRate, str, 10);
		SetDlgItemText(hDlg, IDC_OPTION_REFRATE, str);
		CheckDlgButton(hDlg, IDC_OPTION_REFRATEAUTO, g_bRefRateAuto ? 1 : 0);
		if (g_bRefRateAuto)
			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_REFRATE), false);
		else
			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_REFRATE), true);

		CheckDlgButton(hDlg, IDC_OPTION_OVERCLOCKLOAD, g_bOverClockLoad ? 1 : 0);
		CheckDlgButton(hDlg, IDC_OPTION_CMTADDBLANK, g_bCmtAddBlank ? 1 : 0);
		SendMessage(hBar3, TBM_SETRANGE, TRUE, MAKELPARAM(10, 100));
		SendMessage(hBar3, TBM_SETTICFREQ, 10, 0);
		SendMessage(hBar3, TBM_SETPOS, TRUE, pos3);
		SendMessage(hBar3, TBM_SETPAGESIZE, 0, 10);

		wsprintf(str, _T("%d ms"), pos3);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_QUICKTYPEVIEW), (LPCTSTR)str);

		return (INT_PTR)TRUE;
		break;
	}
	case WM_HSCROLL:
		if (GetDlgItem(hDlg, IDC_OPTION_QUICKTYPE) == (HWND)lParam)
		{
			pos3 = (int)SendMessage(hBar3, TBM_GETPOS, NULL, NULL);
		}
		wsprintf(str, _T("%d ms"), pos3);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_QUICKTYPEVIEW), (LPCTSTR)str);
		return (INT_PTR)TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			HWND hCmbo = GetDlgItem(hDlg, IDC_OPTOIN_LANGUAGE);
			g_language = (int)SendMessage(hCmbo, CB_GETCURSEL, 0, (LPARAM)0);

			TCHAR buff[5];
			TCHAR *e;
			GetDlgItemText(hDlg, IDC_OPTION_REFRATE, buff, 5);
			int r = _tcstol(buff, &e, 10);
			if (r == 0 && *e != '\0') {
				MessageBox(hDlg, g_strTable[(int)Msg::Refresh_rate_incorrect], NULL, 0);
				break;
			}
			else {
				g_refRate = r;
			}
			
			g_keyboard = (IsDlgButtonChecked(hDlg, IDC_OPTION_KBDJP) ? 0 : 1);
			g_bRefRateAuto = (IsDlgButtonChecked(hDlg, IDC_OPTION_REFRATEAUTO) ? true : false);
			g_quickTypeS = (int)SendMessage(hBar3, TBM_GETPOS, NULL, NULL);
			g_bOverClockLoad = (IsDlgButtonChecked(hDlg, IDC_OPTION_OVERCLOCKLOAD) ? true : false);
			g_bCmtAddBlank = (IsDlgButtonChecked(hDlg, IDC_OPTION_CMTADDBLANK) ? true : false);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		}
		case IDC_OPTION_REFRATEAUTO:
			if (g_bRefRateAuto) {
				g_bRefRateAuto = false;
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_REFRATE), true);
			}
			else {
				g_bRefRateAuto = true;
				int r = GetRefreshRate();
				if (r == 0 || r == 1)
					g_refRate = 60;
				else
					g_refRate = r;
				_itot(g_refRate, str, 10);
				SetDlgItemText(hDlg, IDC_OPTION_REFRATE, str);
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_REFRATE), false);
			}
			break;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		case IDC_OPTION_DEFAULT:
			HWND hCmbo = GetDlgItem(hDlg, IDC_OPTOIN_LANGUAGE);
			SendMessage(hCmbo, CB_SETCURSEL, 0, 0);
			CheckDlgButton(hDlg, IDC_OPTION_KBDJP, 1);
			CheckDlgButton(hDlg, IDC_OPTION_KBDEN, 0);

			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_REFRATE), false);
			CheckDlgButton(hDlg, IDC_OPTION_REFRATEAUTO, 1);

			CheckDlgButton(hDlg, IDC_OPTION_OVERCLOCKLOAD, 0);
			CheckDlgButton(hDlg, IDC_OPTION_CMTADDBLANK, 1);

			SendMessage(hBar3, TBM_SETRANGE, TRUE, MAKELPARAM(10, 100));
			SendMessage(hBar3, TBM_SETTICFREQ, 10, 0);
			SendMessage(hBar3, TBM_SETPOS, TRUE, 30);
			SendMessage(hBar3, TBM_SETPAGESIZE, 0, 10);
			wsprintf(str, _T("%d ms"), 30);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_QUICKTYPEVIEW), (LPCTSTR)str);
			break;
		}
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// システム設定タブ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK Tab2Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	const int listLlines = 2;
	const TCHAR* lstStr[listLlines] = {_T("FF FF 00 00 FF FF 00 00"), _T("00 FF 00 FF 00 FF 00 FF")};
	TCHAR str[20];
	int pos1 = g_cpuScale;
	HWND hBar1 = GetDlgItem(hDlg, IDC_OPTION_CPUCLOCK);
	HWND hInitList = GetDlgItem(hDlg, IDC_OPTION_RAM_INIT_PATTERN);

	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
	{
		CheckDlgButton(hDlg, IDC_OPTION_RAMEXPAND1, g_bRamExpand1 ? 1 : 0);
		CheckDlgButton(hDlg, IDC_OPTION_RAMEXPAND2, g_bRamExpand2 ? 1 : 0);
		SendMessage(hBar1, TBM_SETRANGE, TRUE, MAKELPARAM(50, CPU_SPEED_MAX));
		SendMessage(hBar1, TBM_SETTICFREQ, 10, 0);
		SendMessage(hBar1, TBM_SETPOS, TRUE, pos1); 
		SendMessage(hBar1, TBM_SETPAGESIZE, 0, 10);
		wsprintf(str, _T("%d %%"), pos1);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_CPUCLOCKVIEW), (LPCTSTR)str);

		for (int i = 0; i < listLlines; ++i) {
			SendMessage(hInitList, LB_ADDSTRING, 0, (LPARAM)lstStr[i]);
		}
		SendMessage(hInitList, LB_SETCURSEL, (WPARAM)g_ramInitPattern, 0);


		return (INT_PTR)TRUE;
		break;
	}
	case WM_HSCROLL:
		if (GetDlgItem(hDlg, IDC_OPTION_CPUCLOCK) == (HWND)lParam)
		{
			pos1 = (int)SendMessage(hBar1, TBM_GETPOS, NULL, NULL);
		}
		wsprintf(str, _T("%d %%"), pos1);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_CPUCLOCKVIEW), (LPCTSTR)str);
		return (INT_PTR)TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			g_cpuScale = (int)SendMessage(hBar1, TBM_GETPOS, NULL, NULL);
			g_bRamExpand1 = (IsDlgButtonChecked(hDlg, IDC_OPTION_RAMEXPAND1) ? true : false);
			g_bRamExpand2 = (IsDlgButtonChecked(hDlg, IDC_OPTION_RAMEXPAND2) ? true : false);
			g_ramInitPattern = (int)SendMessage(hInitList, LB_GETCURSEL, NULL, NULL);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		case IDC_OPTION_CPU100:
			SendMessage(hBar1, TBM_SETPOS, TRUE, 100);
			wsprintf(str, _T("%d %%"), 100);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_CPUCLOCKVIEW), (LPCTSTR)str);

			break;
		case IDC_OPTION_DEFAULT:
			CheckDlgButton(hDlg, IDC_OPTION_RAMEXPAND1, 0);
			CheckDlgButton(hDlg, IDC_OPTION_RAMEXPAND2, 0);

			SendMessage(hBar1, TBM_SETRANGE, TRUE, MAKELPARAM(50, CPU_SPEED_MAX));
			SendMessage(hBar1, TBM_SETTICFREQ, 10, 0);
			SendMessage(hBar1, TBM_SETPOS, TRUE, 100);
			SendMessage(hBar1, TBM_SETPAGESIZE, 0, 10);
			wsprintf(str, _T("%d %%"), 100);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_CPUCLOCKVIEW), (LPCTSTR)str);
			SendMessage(hInitList, LB_SETCURSEL, 0, (LPARAM)0);
			break;
		}
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド設定タブ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK Tab3Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR str[20];
	int pos2 = g_soundVolume;
	int stereo1 = g_stereo1, stereo2 = g_stereo2, stereo3 = g_stereo3, sBufferWriteInterval = g_sBufferWriteInterval;
	HWND hBar2 = GetDlgItem(hDlg, IDC_OPTION_VOLUME);
	HWND hStereo1 = GetDlgItem(hDlg, IDC_OPTION_STEREO1);
	HWND hStereo2 = GetDlgItem(hDlg, IDC_OPTION_STEREO2);
	HWND hStereo3 = GetDlgItem(hDlg, IDC_OPTION_STEREO3);
	HWND hWriteInterval = GetDlgItem(hDlg, IDC_OPTION_WRITEINTERVAL);
	HWND hBgplay = GetDlgItem(hDlg, IDC_OPTION_BGPLAY);

	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		SendMessage(hBar2, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
		SendMessage(hBar2, TBM_SETTICFREQ, 10, 0);
		SendMessage(hBar2, TBM_SETPOS, TRUE, pos2);
		SendMessage(hBar2, TBM_SETPAGESIZE, 0, 10);

		SendMessage(hStereo1, TBM_SETRANGE, TRUE, MAKELPARAM(0, 10));
		SendMessage(hStereo1, TBM_SETTICFREQ, 5, 0);
		SendMessage(hStereo1, TBM_SETPOS, TRUE, stereo1);
		SendMessage(hStereo1, TBM_SETPAGESIZE, 0, 1);

		SendMessage(hStereo2, TBM_SETRANGE, TRUE, MAKELPARAM(0, 10));
		SendMessage(hStereo2, TBM_SETTICFREQ, 5, 0);
		SendMessage(hStereo2, TBM_SETPOS, TRUE, stereo2);
		SendMessage(hStereo2, TBM_SETPAGESIZE, 0, 1);

		SendMessage(hStereo3, TBM_SETRANGE, TRUE, MAKELPARAM(0, 10));
		SendMessage(hStereo3, TBM_SETTICFREQ, 5, 0);
		SendMessage(hStereo3, TBM_SETPOS, TRUE, stereo3);
		SendMessage(hStereo3, TBM_SETPAGESIZE, 0, 1);

		SendMessage(hWriteInterval, TBM_SETRANGE, TRUE, MAKELPARAM(0, 5));
		SendMessage(hWriteInterval, TBM_SETTICFREQ, 1, 0);
		SendMessage(hWriteInterval, TBM_SETPOS, TRUE, (100 - sBufferWriteInterval) / 10 );
		SendMessage(hWriteInterval, TBM_SETPAGESIZE, 0, 1);

		wsprintf(str, _T("%d %%"), pos2);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_VOLUMEVIEW), (LPCTSTR)str);

		wsprintf(str, _T("1/%ds"), sBufferWriteInterval);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_WRITEINTERVALVIEW), (LPCTSTR)str);

		CheckDlgButton(hDlg, IDC_OPTION_BGPLAY, g_bBGPlay ? 1 : 0);

		return (INT_PTR)TRUE;
		break;
	case WM_HSCROLL:
		if (GetDlgItem(hDlg, IDC_OPTION_VOLUME) == (HWND)lParam)
		{
			pos2 = (int)SendMessage(hBar2, TBM_GETPOS, NULL, NULL);
			wsprintf(str, _T("%d %%"), pos2);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_VOLUMEVIEW), (LPCTSTR)str);
		}

		if (GetDlgItem(hDlg, IDC_OPTION_STEREO1) == (HWND)lParam)
		{
			stereo1 = (int)SendMessage(hStereo1, TBM_GETPOS, NULL, NULL);
		}
		if (GetDlgItem(hDlg, IDC_OPTION_STEREO2) == (HWND)lParam)
		{
			stereo2 = (int)SendMessage(hStereo2, TBM_GETPOS, NULL, NULL);
		}
		if (GetDlgItem(hDlg, IDC_OPTION_STEREO3) == (HWND)lParam)
		{
			stereo3 = (int)SendMessage(hStereo3, TBM_GETPOS, NULL, NULL);
		}
		if (GetDlgItem(hDlg, IDC_OPTION_WRITEINTERVAL) == (HWND)lParam)
		{
			sBufferWriteInterval = (int)SendMessage(hWriteInterval, TBM_GETPOS, NULL, NULL);
			wsprintf(str, _T("1/%ds"), 100 - sBufferWriteInterval * 10);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_WRITEINTERVALVIEW), (LPCTSTR)str);
		}

		return (INT_PTR)TRUE;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			g_soundVolume = (int)SendMessage(hBar2, TBM_GETPOS, NULL, NULL);
			sys.pMn1271->SetVolume(g_soundVolume);

			g_stereo1 = (int)SendMessage(hStereo1, TBM_GETPOS, NULL, NULL);
			g_stereo2 = (int)SendMessage(hStereo2, TBM_GETPOS, NULL, NULL);
			g_stereo3 = (int)SendMessage(hStereo3, TBM_GETPOS, NULL, NULL);
			g_sBufferWriteInterval = 100 - 10 * (int)SendMessage(hWriteInterval, TBM_GETPOS, NULL, NULL);
			sys.pMn1271->SetPan(2, g_stereo1);
			sys.pMn1271->SetPan(0, g_stereo2);
			sys.pMn1271->SetPan(1, g_stereo3);
			g_bBGPlay = (IsDlgButtonChecked(hDlg, IDC_OPTION_BGPLAY) ? true : false);

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		case IDC_OPTION_DEFAULT:
			SendMessage(hBar2, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendMessage(hBar2, TBM_SETTICFREQ, 10, 0);
			SendMessage(hBar2, TBM_SETPOS, TRUE, 40);
			SendMessage(hBar2, TBM_SETPAGESIZE, 0, 10);

			SendMessage(hStereo1, TBM_SETRANGE, TRUE, MAKELPARAM(0, 10));
			SendMessage(hStereo1, TBM_SETTICFREQ, 10, 0);
			SendMessage(hStereo1, TBM_SETPOS, TRUE, 5);
			SendMessage(hStereo1, TBM_SETPAGESIZE, 0, 1);

			SendMessage(hStereo2, TBM_SETRANGE, TRUE, MAKELPARAM(0, 10));
			SendMessage(hStereo2, TBM_SETTICFREQ, 10, 0);
			SendMessage(hStereo2, TBM_SETPOS, TRUE, 5);
			SendMessage(hStereo2, TBM_SETPAGESIZE, 0, 1);

			SendMessage(hStereo3, TBM_SETRANGE, TRUE, MAKELPARAM(0, 10));
			SendMessage(hStereo3, TBM_SETTICFREQ, 10, 0);
			SendMessage(hStereo3, TBM_SETPOS, TRUE, 5);
			SendMessage(hStereo3, TBM_SETPAGESIZE, 0, 1);

			SendMessage(hWriteInterval, TBM_SETRANGE, TRUE, MAKELPARAM(0, 5));
			SendMessage(hWriteInterval, TBM_SETTICFREQ, 1, 0);
			SendMessage(hWriteInterval, TBM_SETPOS, TRUE, 4);
			SendMessage(hWriteInterval, TBM_SETPAGESIZE, 0, 1);

			wsprintf(str, _T("%d %%"), 40);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_VOLUMEVIEW), (LPCTSTR)str);

			wsprintf(str, _T("1/%ds"), 60);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_WRITEINTERVALVIEW), (LPCTSTR)str);

			CheckDlgButton(hDlg, IDC_OPTION_BGPLAY, 0);

			return (INT_PTR)TRUE;
			break;
		}
	}
	return (INT_PTR)FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// プリンタ設定タブ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK Tab4Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
	{
		if (g_prOutput == 0) { // TEXT
			CheckDlgButton(hDlg, IDC_OPTION_PRTEXT, 1);
			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), FALSE);
		}
		else if (g_prOutput == 1){ //RAW
			CheckDlgButton(hDlg, IDC_OPTION_PRRAW, 1);
		}
		else { //PNG
			CheckDlgButton(hDlg, IDC_OPTION_PRPNG, 1);
			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), FALSE);
		}

		if (g_prMaker == 0) {
			CheckDlgButton(hDlg, IDC_OPTION_EPSON, 1);
		}
		else {
			CheckDlgButton(hDlg, IDC_OPTION_PANA, 1);
		}

		CheckDlgButton(hDlg, IDC_OPTION_PRAUTOFEED, g_prAutoFeed);

		return (INT_PTR)TRUE;
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			if (IsDlgButtonChecked(hDlg, IDC_OPTION_PRTEXT)) {
				g_prOutput = (int)PrinterOutput::TEXT;
				delete sys.pPrinter;
				sys.pPrinter = new TextPrinter();
				sys.pPrinter->Init();
			}
			else if (IsDlgButtonChecked(hDlg, IDC_OPTION_PRRAW)) {
				g_prOutput = (int)PrinterOutput::RAW;
				delete sys.pPrinter;
				sys.pPrinter = new RawPrinter();
				sys.pPrinter->Init();
			}
			else {
				g_prOutput = (int)PrinterOutput::PNG;
				delete sys.pPrinter;
				sys.pPrinter = new ImagePrinter();
				sys.pPrinter->Init();
			}

			g_prMaker = (IsDlgButtonChecked(hDlg, IDC_OPTION_EPSON) ? 0 : 1);
			g_prAutoFeed = IsDlgButtonChecked(hDlg, IDC_OPTION_PRAUTOFEED);
			
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		case IDC_OPTION_DEFAULT:
			CheckDlgButton(hDlg, IDC_OPTION_PRTEXT, 1);
			CheckDlgButton(hDlg, IDC_OPTION_PRRAW, 0);
			CheckDlgButton(hDlg, IDC_OPTION_PRPNG, 0);
			CheckDlgButton(hDlg, IDC_OPTION_EPSON, 1);
			CheckDlgButton(hDlg, IDC_OPTION_PANA, 0);
			CheckDlgButton(hDlg, IDC_OPTION_PRAUTOFEED, 1);
			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), FALSE);
			break;
		case IDC_OPTION_PRTEXT:
			if (IsDlgButtonChecked(hDlg, IDC_OPTION_PRTEXT)) {
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), FALSE);
			}
			else {
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), TRUE);
			}
			break;
		case IDC_OPTION_PRRAW:
			if (IsDlgButtonChecked(hDlg, IDC_OPTION_PRRAW)) {
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), TRUE);
			}
			else {
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), FALSE);
			}
			break;
		case IDC_OPTION_PRPNG:
			if (IsDlgButtonChecked(hDlg, IDC_OPTION_PRPNG)) {
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), FALSE);
			}
			else {
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_PANA), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_OPTION_EPSON), TRUE);
			}
			break;

		}
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// キーフック　コールバック関数
//
////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(hHook, nCode, wParam, lParam);

	if (keyDefPressed == 1) {
		TCHAR str[10];
		_itot((int)wParam, str, 10);
		SetWindowText(GetDlgItem(hDlg5, IDC_OPTION_FORCEDACODE), str);
		keyDefPressed = 0;
		SetWindowText(GetDlgItem(hDlg5, IDC_OPTION_JOYPADMESSAGE), _T(""));
	}
	else if (keyDefPressed == 2) {
		TCHAR str[10];
		_itot((int)wParam, str, 10);
		SetWindowText(GetDlgItem(hDlg5, IDC_OPTION_FORCEDBCODE), str);
		keyDefPressed = 0;
		SetWindowText(GetDlgItem(hDlg5, IDC_OPTION_JOYPADMESSAGE), _T(""));
	}
	UnhookWindowsHookEx(hHook);
	hHook = NULL;
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ジョイパッド設定タブ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK Tab5Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message) {
	case WM_INITDIALOG:
	{
		info.dwSize = sizeof(JOYINFOEX);
		info.dwFlags = JOY_RETURNBUTTONS;

		if (g_b2buttons == 0) {
			CheckDlgButton(hDlg, IDC_OPTION_JOYPAD1BTN, 1);
		}
		else {
			CheckDlgButton(hDlg, IDC_OPTION_JOYPAD2BTN, 1);
		}

		TCHAR str[10];
		_itot(g_Joypad1pA, str, 10);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD1ACODE), str);
		_itot(g_Joypad1pB, str, 10);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD1BCODE), str);
		_itot(g_Joypad2pA, str, 10);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD2ACODE), str);
		_itot(g_Joypad2pB, str, 10);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD2BCODE), str);
		_itot(g_forcedJoypadA, str, 10);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_FORCEDACODE), str);
		_itot(g_forcedJoypadB, str, 10);
		SetWindowText(GetDlgItem(hDlg, IDC_OPTION_FORCEDBCODE), str);

		return (INT_PTR)TRUE;
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			g_b2buttons = (IsDlgButtonChecked(hDlg, IDC_OPTION_JOYPAD1BTN) ? 0 : 1);
			const int LEN = 10;
			TCHAR str[LEN];
			GetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD1ACODE), str, LEN);
			g_Joypad1pA = _ttoi(str);
			GetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD1BCODE), str, LEN);
			g_Joypad1pB = _ttoi(str);
			GetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD2ACODE), str, LEN);
			g_Joypad2pA = _ttoi(str);
			GetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD2BCODE), str, LEN);
			g_Joypad2pB = _ttoi(str);

			GetWindowText(GetDlgItem(hDlg, IDC_OPTION_FORCEDACODE), str, LEN);
			g_forcedJoypadA = _ttoi(str);
			GetWindowText(GetDlgItem(hDlg, IDC_OPTION_FORCEDBCODE), str, LEN);
			g_forcedJoypadB = _ttoi(str);

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		case IDC_OPTION_DEFAULT:
			CheckDlgButton(hDlg, IDC_OPTION_JOYPAD1BTN, 1);
			CheckDlgButton(hDlg, IDC_OPTION_JOYPAD2BTN, 0);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD1ACODE), _T("0"));
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD1BCODE), _T("0"));
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD2ACODE), _T("0"));
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD2BCODE), _T("0"));
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_FORCEDACODE), _T("32"));
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_FORCEDBCODE), _T("32"));
			break;
		case IDC_OPTION_1PA:
			if (buttonDefPressed == 0) {
				SetTimer(hDlg, 1, 100, NULL);
				buttonDefPressed = 1;
				SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), g_strTable[(int)Msg::Press_the_joypad_button]);
			}
			break;
		case IDC_OPTION_1PB:
			if (buttonDefPressed == 0) {
				SetTimer(hDlg, 1, 100, NULL);
				buttonDefPressed = 2;
				SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), g_strTable[(int)Msg::Press_the_joypad_button]);
			}
			break;
		case IDC_OPTION_2PA:
			if (buttonDefPressed == 0) {
				SetTimer(hDlg, 1, 100, NULL);
				buttonDefPressed = 3;
				SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), g_strTable[(int)Msg::Press_the_joypad_button]);
			}
			break;
		case IDC_OPTION_2PB:
			if (buttonDefPressed == 0) {
				SetTimer(hDlg, 1, 100, NULL);
				buttonDefPressed = 4;
				SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), g_strTable[(int)Msg::Press_the_joypad_button]);
			}
			break;
		case IDC_OPTION_FORCEDA:
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), g_strTable[(int)Msg::Press_the_key_to_assign]);
			keyDefPressed = 1;
			hDlg5 = hDlg;
			hHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, NULL, GetCurrentThreadId());
			break;
		case IDC_OPTION_FORCEDB:
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), g_strTable[(int)Msg::Press_the_key_to_assign]);
			keyDefPressed = 2;
			hDlg5 = hDlg;
			hHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, NULL, GetCurrentThreadId());
			break;
		}
		return (INT_PTR)TRUE;
		break;
	case WM_TIMER:
		switch (buttonDefPressed)
		{
		case 1:
			if (joyGetPosEx(JOYSTICKID1, &info) == JOYERR_NOERROR) {
				if (info.dwButtons != 0) {
					TCHAR str[10];
					_itot(info.dwButtons, str, 10);
					SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD1ACODE), str);
					KillTimer(hDlg, 1);
					SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), _T(""));
					buttonDefPressed = 0;
				}
			}
			break;
		case 2:
			if (joyGetPosEx(JOYSTICKID1, &info) == JOYERR_NOERROR) {
				if (info.dwButtons != 0) {
					TCHAR str[10];
					_itot(info.dwButtons, str, 10);
					SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD1BCODE), str);
					KillTimer(hDlg, 1);
					SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), _T(""));
					buttonDefPressed = 0;
				}
			}
			break;
		case 3:
			if (joyGetPosEx(JOYSTICKID2, &info) == JOYERR_NOERROR) {
				if (info.dwButtons != 0) {
					TCHAR str[10];
					_itot(info.dwButtons, str, 10);
					SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD2ACODE), str);
					KillTimer(hDlg, 1);
					SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), _T(""));
					buttonDefPressed = 0;
				}
			}
			break;
		case 4:
			if (joyGetPosEx(JOYSTICKID2, &info) == JOYERR_NOERROR) {
				if (info.dwButtons != 0) {
					TCHAR str[10];
					_itot(info.dwButtons, str, 10);
					SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPAD2BCODE), str);
					KillTimer(hDlg, 1);
					SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), _T(""));
					buttonDefPressed = 0;
				}
			};
			break;
		}
		return (INT_PTR)TRUE;
		break;
	case WM_LBUTTONDOWN:
		if (buttonDefPressed != 0) {
			KillTimer(hDlg, 1);
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), _T(""));
			buttonDefPressed = 0;
		}
		if (keyDefPressed != 0) {
			if (hHook != NULL) {
				UnhookWindowsHookEx(hHook);
				hHook = NULL;
			}
			SetWindowText(GetDlgItem(hDlg, IDC_OPTION_JOYPADMESSAGE), _T(""));
			keyDefPressed = 0;
		}

		return (INT_PTR)TRUE;
		break;
	case WM_DESTROY:
		if (buttonDefPressed != 0)
			KillTimer(hDlg, 1);

		if (hHook != NULL) {
			UnhookWindowsHookEx(hHook);
			hHook = NULL;
		}
		break;
	}
	return (INT_PTR)FALSE;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// オプションダイアログ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK DlgOptionProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hBar1 = GetDlgItem(hDlg, IDC_OPTION_CPUCLOCK);
	HWND hBar2 = GetDlgItem(hDlg, IDC_OPTION_VOLUME);
	HWND hBar3 = GetDlgItem(hDlg, IDC_OPTION_QUICKTYPE);
	int pos1 = g_cpuScale;
	int pos2 = g_soundVolume;
	int pos3 = g_quickTypeS;

	switch (message) {
	case WM_INITDIALOG:
	{
		hTab = GetDlgItem(hDlg, IDC_OPTION_TABCON);

		//タブ追加
		TC_ITEM TabItem;

		TabItem.mask = TCIF_TEXT;
		TabItem.pszText = g_strTable[(int)Msg::Emulator];
		TabCtrl_InsertItem(hTab, ID_TAB1, &TabItem);

		TabItem.mask = TCIF_TEXT;
		TabItem.pszText = g_strTable[(int)Msg::System];
		TabCtrl_InsertItem(hTab, ID_TAB2, &TabItem);

		TabItem.mask = TCIF_TEXT;
		TabItem.pszText = g_strTable[(int)Msg::Sound];
		TabCtrl_InsertItem(hTab, ID_TAB3, &TabItem);

		TabItem.mask = TCIF_TEXT;
		TabItem.pszText = g_strTable[(int)Msg::Printer];
		TabCtrl_InsertItem(hTab, ID_TAB4, &TabItem);

		TabItem.mask = TCIF_TEXT;
		TabItem.pszText = g_strTable[(int)Msg::Joypad]; //////////////////////
		TabCtrl_InsertItem(hTab, ID_TAB5, &TabItem);

		//タブ(ダイアログ)作成
		hTab1 = CreateDialog(g_hMod, MAKEINTRESOURCE(IDD_OPTIONTAB1), hDlg, (DLGPROC)Tab1Proc);
		hTab2 = CreateDialog(g_hMod, MAKEINTRESOURCE(IDD_OPTIONTAB2), hDlg, (DLGPROC)Tab2Proc);
		hTab3 = CreateDialog(g_hMod, MAKEINTRESOURCE(IDD_OPTIONTAB3), hDlg, (DLGPROC)Tab3Proc);
		hTab4 = CreateDialog(g_hMod, MAKEINTRESOURCE(IDD_OPTIONTAB4), hDlg, (DLGPROC)Tab4Proc);
		hTab5 = CreateDialog(g_hMod, MAKEINTRESOURCE(IDD_OPTIONTAB5), hDlg, (DLGPROC)Tab5Proc);

		//タブの座標取得
		RECT rc;
		LPPOINT lpt = (LPPOINT)&rc;

		GetClientRect(hTab, &rc);
		TabCtrl_AdjustRect(hTab, false, &rc);
		//座標変換
		MapWindowPoints(hTab, hDlg, lpt, 2);

		//タブの位置とサイズの調整
		MoveWindow(hTab1,
			rc.left,
			rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			false
		);
		MoveWindow(hTab2,
			rc.left,
			rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			false
		);
		MoveWindow(hTab3,
			rc.left,
			rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			false
		);
		MoveWindow(hTab4,
			rc.left,
			rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			false
		);
		MoveWindow(hTab5,
			rc.left,
			rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			false
		);

		//タブを表示
		ShowWindow(hTab1, SW_SHOW);
		ShowWindow(hTab2, SW_HIDE);
		ShowWindow(hTab3, SW_HIDE);
		ShowWindow(hTab4, SW_HIDE);
		ShowWindow(hTab5, SW_HIDE);
		TabCtrl_SetCurFocus(hTab, ID_TAB1);
		return (INT_PTR)TRUE;
		break;
	}
	case WM_NOTIFY: 
	{
		switch (((LPNMHDR)lParam)->idFrom) {
		case IDC_OPTION_TABCON:
			switch (((LPNMHDR)lParam)->code) {
			case TCN_SELCHANGE:
				switch (TabCtrl_GetCurSel(hTab)) {
				case ID_TAB1:
					ShowWindow(hTab1, SW_SHOW);
					ShowWindow(hTab2, SW_HIDE);
					ShowWindow(hTab3, SW_HIDE);
					ShowWindow(hTab4, SW_HIDE);
					ShowWindow(hTab5, SW_HIDE);
					if (hHook != NULL) {
						UnhookWindowsHookEx(hHook);
						hHook = NULL;
					}
					return TRUE;
				case ID_TAB2:
					ShowWindow(hTab1, SW_HIDE);
					ShowWindow(hTab2, SW_SHOW);
					ShowWindow(hTab3, SW_HIDE);
					ShowWindow(hTab4, SW_HIDE);
					ShowWindow(hTab5, SW_HIDE);
					if (hHook != NULL) {
						UnhookWindowsHookEx(hHook);
						hHook = NULL;
					}
					return TRUE;
				case ID_TAB3:
					ShowWindow(hTab1, SW_HIDE);
					ShowWindow(hTab2, SW_HIDE);
					ShowWindow(hTab3, SW_SHOW);
					ShowWindow(hTab4, SW_HIDE);
					ShowWindow(hTab5, SW_HIDE);
					if (hHook != NULL) {
						UnhookWindowsHookEx(hHook);
						hHook = NULL;
					}
					return TRUE;
				case ID_TAB4:
					ShowWindow(hTab1, SW_HIDE);
					ShowWindow(hTab2, SW_HIDE);
					ShowWindow(hTab3, SW_HIDE);
					ShowWindow(hTab4, SW_SHOW);
					ShowWindow(hTab5, SW_HIDE);
					if (hHook != NULL) {
						UnhookWindowsHookEx(hHook);
						hHook = NULL;
					}
					return TRUE;
				case ID_TAB5:
					ShowWindow(hTab1, SW_HIDE);
					ShowWindow(hTab2, SW_HIDE);
					ShowWindow(hTab3, SW_HIDE);
					ShowWindow(hTab4, SW_HIDE);
					ShowWindow(hTab5, SW_SHOW);
					return TRUE;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
		return (INT_PTR)TRUE;
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)){
		case IDOK:
		{
			SendMessage(hTab1, WM_COMMAND, IDOK, 0);
			SendMessage(hTab2, WM_COMMAND, IDOK, 0);
			SendMessage(hTab3, WM_COMMAND, IDOK, 0);
			SendMessage(hTab4, WM_COMMAND, IDOK, 0);
			SendMessage(hTab5, WM_COMMAND, IDOK, 0);

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		case IDC_OPTION_DEFAULT:
			SendMessage(hTab1, WM_COMMAND, IDC_OPTION_DEFAULT, 0);
			SendMessage(hTab2, WM_COMMAND, IDC_OPTION_DEFAULT, 0);
			SendMessage(hTab3, WM_COMMAND, IDC_OPTION_DEFAULT, 0);
			SendMessage(hTab4, WM_COMMAND, IDC_OPTION_DEFAULT, 0);
			SendMessage(hTab5, WM_COMMAND, IDC_OPTION_DEFAULT, 0);
			return (INT_PTR)TRUE;
			break;
		}
		return (INT_PTR)TRUE;
		break;
	}
	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return (INT_PTR)TRUE;
		break;
	default:
		return (INT_PTR)FALSE;
	}
	return (INT_PTR)FALSE;
}


