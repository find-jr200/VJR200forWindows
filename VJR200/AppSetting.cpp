// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class AppSetting
// INIファイルの読み出し、書き込みを担当
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <Shlobj.h>
#include <Shlwapi.h>
#include "VJR200.h"
#include "AppSetting.h"

using namespace std;

#pragma comment(lib, "Shlwapi.lib")

AppSetting::AppSetting()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AppSetting::Init()
{
	SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, fileName);
	wcscat(fileName, L"\\FIND_JR");
	if (!PathFileExists(fileName))
		CreateDirectory(fileName, 0);
	wcscat(fileName, L"\\VJR200");
	if (!PathFileExists(fileName))
		CreateDirectory(fileName, 0);
	wcscat(fileName, L"\\vjr200.ini");

	if (!PathFileExists(fileName))
		return false;
	else
		return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// INIファイル読込
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AppSetting::Read()
{
	TCHAR buff[MAX_PATH] = {};

	g_windowPos.left = GetPrivateProfileInt(_T("Window"), _T("PosX"), 100, fileName);
	g_windowPos.top = GetPrivateProfileInt(_T("Window"), _T("PosY"), 100, fileName);
	g_viewScale = GetPrivateProfileInt(_T("Window"), _T("viewScale"), 2, fileName);
	g_bSquarePixel = GetPrivateProfileInt(_T("Window"), _T("SquarePixel"), 0, fileName) == 1 ? true : false;
	g_bSmoothing = GetPrivateProfileInt(_T("Window"), _T("Smoothing"), 0, fileName) == 1 ? true : false;
	g_bPrinterLed = GetPrivateProfileInt(_T("Window"), _T("PrinterLed"), 1, fileName) == 1 ? true : false;
	g_bStatusBar = GetPrivateProfileInt(_T("Window"), _T("StatusBar"), 1, fileName) == 1 ? true : false;

	GetPrivateProfileString(_T("Path"), _T("RomFile"), _T(""), buff, MAX_PATH, fileName);
	_tcscpy(g_pRomFile, buff);
	GetPrivateProfileString(_T("Path"), _T("FontFile"), _T(""), buff, MAX_PATH, fileName);
	_tcscpy(g_pFontFile, buff);

	TCHAR c[5], tmpBuf[MAX_PATH];
	tmpBuf[0] = '\0';
	for (unsigned int i = 0; i < RECENT_FILES_NUM; ++i) {
		_tcscat(tmpBuf, _T("RFilesforCJRSet"));
		_itot(i, c, 10);
		_tcscat(tmpBuf, c);
		GetPrivateProfileString(_T("Path"), tmpBuf, _T(""), buff, MAX_PATH, fileName);
		g_rFilesforCJRSet.push_back(buff);
		tmpBuf[0] = '\0';
	}

	for (unsigned int i = 0; i < RECENT_FILES_NUM; ++i) {
		_tcscat(tmpBuf, _T("RFilesforQLoad"));
		_itot(i, c, 10);
		_tcscat(tmpBuf, c);
		GetPrivateProfileString(_T("Path"), tmpBuf, _T(""), buff, MAX_PATH, fileName);
		g_rFilesforQLoad.push_back(buff);
		tmpBuf[0] = '\0';
	}

	g_bFpsCpu = GetPrivateProfileInt(_T("Options"), _T("fpsCpu"), 0, fileName) == 1 ? true : false;
	g_bRamExpand1 = GetPrivateProfileInt(_T("Options"), _T("ramExpand1"), 0, fileName) == 1 ? true : false;
	g_bRamExpand2 = GetPrivateProfileInt(_T("Options"), _T("ramExpand2"), 0, fileName) == 1 ? true : false;
	g_ramInitPattern = GetPrivateProfileInt(_T("Options"), _T("ramInitPattern"), 0, fileName);
	g_refRate = GetPrivateProfileInt(_T("Options"), _T("refRate"), 0, fileName);
	g_bRefRateAuto = GetPrivateProfileInt(_T("Options"), _T("refRateAuto"), 0, fileName) == 1 ? true : false;
	g_soundVolume = GetPrivateProfileInt(_T("Options"), _T("soundVolume"), 40, fileName);
	g_bOverClockLoad = GetPrivateProfileInt(_T("Options"), _T("overClockLoad"), 0, fileName) == 1 ? true : false;
	g_bCmtAddBlank = GetPrivateProfileInt(_T("Options"), _T("cmtAddBlank"), 1, fileName) == 1 ? true : false;
	g_quickTypeS = GetPrivateProfileInt(_T("Options"), _T("quickType"), 30, fileName);
	g_language = GetPrivateProfileInt(_T("Options"), _T("language"), 0, fileName);
	g_keyboard = GetPrivateProfileInt(_T("Options"), _T("keyboard"), 0, fileName);
	g_bRomajiKana = GetPrivateProfileInt(_T("Options"), _T("romajiKana"), 0, fileName) == 1 ? true : false;

	g_stereo1 = GetPrivateProfileInt(_T("Sound"), _T("stereo1"), 5, fileName);
	g_stereo2 = GetPrivateProfileInt(_T("Sound"), _T("stereo2"), 5, fileName);
	g_stereo3 = GetPrivateProfileInt(_T("Sound"), _T("stereo3"), 5, fileName);
	g_bBGPlay = GetPrivateProfileInt(_T("Sound"), _T("bgPlay"), 0, fileName) == 1 ? true : false;
	g_sBufferWriteInterval = GetPrivateProfileInt(_T("Sound"), _T("sBufferWriteInterval"), 60, fileName);

	g_prOutput = GetPrivateProfileInt(_T("Printer"), _T("output"), 0, fileName);
	g_prMaker = GetPrivateProfileInt(_T("Printer"), _T("maker"), 0, fileName);
	g_prAutoFeed = GetPrivateProfileInt(_T("Printer"), _T("autoFeed"), 1, fileName);

	g_b2buttons = GetPrivateProfileInt(_T("Joypad"), _T("2buttons"), 0, fileName) == 1 ? true : false;
	g_Joypad1pA = GetPrivateProfileInt(_T("Joypad"), _T("1pAbutton"), 0, fileName);
	g_Joypad1pB = GetPrivateProfileInt(_T("Joypad"), _T("1pBbutton"), 0, fileName);
	g_Joypad2pA = GetPrivateProfileInt(_T("Joypad"), _T("2pAbutton"), 0, fileName);
	g_Joypad2pB = GetPrivateProfileInt(_T("Joypad"), _T("2pBbutton"), 0, fileName);
	g_forcedJoypadA = GetPrivateProfileInt(_T("Joypad"), _T("ForcedJoystickA"), 32, fileName);
	g_forcedJoypadB = GetPrivateProfileInt(_T("Joypad"), _T("ForcedJoystickB"), 32, fileName);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// INIファイル書き込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void AppSetting::Write()
{
	TCHAR buff[MAX_PATH];

	WritePrivateProfileString(_T("Window"), _T("SquarePixcel"), NULL, fileName); // スペルミスなので削除
	WritePrivateProfileString(_T("Options"), _T("g_quickTypeV"), NULL, fileName); // スペルミスなので削除

	wsprintf(buff, _T("%d"), g_windowPos.left);
	WritePrivateProfileString(_T("Window"), _T("PosX"), buff, fileName);
	wsprintf(buff, _T("%d"), g_windowPos.top);
	WritePrivateProfileString(_T("Window"), _T("PosY"), buff, fileName);
	wsprintf(buff, _T("%d"), g_viewScale);
	WritePrivateProfileString(_T("Window"), _T("viewScale"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bSquarePixel);
	WritePrivateProfileString(_T("Window"), _T("SquarePixel"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bSmoothing ? 1 : 0);
	WritePrivateProfileString(_T("Window"), _T("Smoothing"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bPrinterLed ? 1 : 0);
	WritePrivateProfileString(_T("Window"), _T("PrinterLed"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bStatusBar ? 1 : 0);
	WritePrivateProfileString(_T("Window"), _T("StatusBar"), buff, fileName);

	WritePrivateProfileString(_T("Path"), _T("RomFile"), g_pRomFile, fileName);
	WritePrivateProfileString(_T("Path"), _T("FontFile"), g_pFontFile, fileName);

	TCHAR tmpBuf[MAX_PATH];
	tmpBuf[0] = '\0';
	if (g_rFilesforCJRSet.size() >= RECENT_FILES_NUM + 1)
		while (g_rFilesforCJRSet.size() > RECENT_FILES_NUM) g_rFilesforCJRSet.pop_back();
	for (unsigned int i = 0; i < g_rFilesforCJRSet.size(); ++i) {
		wsprintf(tmpBuf, _T("%s%d"), _T("RFilesforCJRSet"), i);
		WritePrivateProfileString(_T("Path"), tmpBuf, g_rFilesforCJRSet[i].data(), fileName);
		tmpBuf[0] = '\0';
	}

	if (g_rFilesforQLoad.size() >= RECENT_FILES_NUM + 1)
		while (g_rFilesforQLoad.size() > RECENT_FILES_NUM) g_rFilesforQLoad.pop_back();
	for (unsigned int i = 0; i < g_rFilesforQLoad.size(); ++i) {
		wsprintf(tmpBuf, _T("%s%d"), _T("RFilesforQLoad"), i);
		WritePrivateProfileString(_T("Path"), tmpBuf, g_rFilesforQLoad[i].data(), fileName);
		tmpBuf[0] = '\0';
	}

	wsprintf(buff, _T("%d"), g_bFpsCpu ? 1 : 0);
	WritePrivateProfileString(_T("Options"), _T("fpsCpu"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bRamExpand1 ? 1 : 0);
	WritePrivateProfileString(_T("Options"), _T("ramExpand1"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bRamExpand2 ? 1 : 0);
	WritePrivateProfileString(_T("Options"), _T("ramExpand2"), buff, fileName);
	wsprintf(buff, _T("%d"), g_ramInitPattern);
	WritePrivateProfileString(_T("Options"), _T("ramInitPattern"), buff, fileName);
	wsprintf(buff, _T("%d"), g_refRate);
	WritePrivateProfileString(_T("Options"), _T("refRate"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bRefRateAuto ? 1 : 0);
	WritePrivateProfileString(_T("Options"), _T("refRateAuto"), buff, fileName);
	wsprintf(buff, _T("%d"), g_soundVolume);
	WritePrivateProfileString(_T("Options"), _T("soundVolume"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bOverClockLoad ? 1 : 0);
	WritePrivateProfileString(_T("Options"), _T("overClockLoad"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bCmtAddBlank ? 1 : 0);
	WritePrivateProfileString(_T("Options"), _T("cmtAddBlank"), buff, fileName);
	wsprintf(buff, _T("%d"), g_quickTypeS);
	WritePrivateProfileString(_T("Options"), _T("quickType"), buff, fileName);
	wsprintf(buff, _T("%d"), g_language);
	WritePrivateProfileString(_T("Options"), _T("language"), buff, fileName);
	wsprintf(buff, _T("%d"), g_keyboard);
	WritePrivateProfileString(_T("Options"), _T("keyboard"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bRomajiKana ? 1 : 0);
	WritePrivateProfileString(_T("Options"), _T("romajiKana"), buff, fileName);

	wsprintf(buff, _T("%d"), g_stereo1);
	WritePrivateProfileString(_T("Sound"), _T("stereo1"), buff, fileName);
	wsprintf(buff, _T("%d"), g_stereo2);
	WritePrivateProfileString(_T("Sound"), _T("stereo2"), buff, fileName);
	wsprintf(buff, _T("%d"), g_stereo3);
	WritePrivateProfileString(_T("Sound"), _T("stereo3"), buff, fileName);
	wsprintf(buff, _T("%d"), g_bBGPlay ? 1 : 0);
	WritePrivateProfileString(_T("Sound"), _T("bgPlay"), buff, fileName);
	wsprintf(buff, _T("%d"), g_sBufferWriteInterval);
	WritePrivateProfileString(_T("Sound"), _T("sBufferWriteInterval"), buff, fileName);

	wsprintf(buff, _T("%d"), g_prOutput);
	WritePrivateProfileString(_T("Printer"), _T("output"), buff, fileName);
	wsprintf(buff, _T("%d"), g_prMaker);
	WritePrivateProfileString(_T("Printer"), _T("maker"), buff, fileName);
	wsprintf(buff, _T("%d"), g_prAutoFeed);
	WritePrivateProfileString(_T("Printer"), _T("autoFeed"), buff, fileName);

	wsprintf(buff, _T("%d"), g_b2buttons ? 1 : 0);
	WritePrivateProfileString(_T("Joypad"), _T("2buttons"), buff, fileName);
	wsprintf(buff, _T("%d"), g_Joypad1pA);
	WritePrivateProfileString(_T("Joypad"), _T("1pAbutton"), buff, fileName);
	wsprintf(buff, _T("%d"), g_Joypad1pB);
	WritePrivateProfileString(_T("Joypad"), _T("1pBbutton"), buff, fileName);
	wsprintf(buff, _T("%d"), g_Joypad2pA);
	WritePrivateProfileString(_T("Joypad"), _T("2pAbutton"), buff, fileName);
	wsprintf(buff, _T("%d"), g_Joypad2pB);
	WritePrivateProfileString(_T("Joypad"), _T("2pBbutton"), buff, fileName);
	wsprintf(buff, _T("%d"), g_forcedJoypadA);
	WritePrivateProfileString(_T("Joypad"), _T("ForcedJoystickA"), buff, fileName);
	wsprintf(buff, _T("%d"), g_forcedJoypadB);
	WritePrivateProfileString(_T("Joypad"), _T("ForcedJoystickB"), buff, fileName);

	WritePrivateProfileString(NULL, NULL, NULL, fileName);
}


AppSetting::~AppSetting()
{
}


