// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class RawPrinter
// RAWプリンタ出力
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "RawPrinter.h"
#include "VJR200.h"
#include "Mn1271.h"
#include <Shlobj.h>
#include <Shlwapi.h>

RawPrinter::RawPrinter()
{
}


RawPrinter::~RawPrinter()
{
	if (hFile != NULL)
		CloseHandle(hFile);

	DeleteObject(bmpLedActive);
	DeleteObject(bmpLedInactive);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool RawPrinter::Init()
{
	SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, NULL, myDocumentsFolder);
	_tcscpy(writePath, myDocumentsFolder);
	_tcscat(writePath, _T("\\JR200Print.bin"));

	bmpLedActive = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PRNRAW_ACTIVE));
	bmpLedInactive = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PRNRAW_INACTIVE));

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ページ送り
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void RawPrinter::Finish(bool bManual)
{
	if (hFile != NULL) {
		CloseHandle(hFile);
		hFile = NULL;
	}
	return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1byte書込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void RawPrinter::Write(uint8_t val)
{
	DataActive();

	DWORD dwNumberOfBytesWritten;

	if (hFile == NULL) {
		if (PathFileExists(writePath) && !PathIsDirectory(writePath)) {
			hFile = CreateFile(writePath,
				FILE_APPEND_DATA,
				0,
				0,
				OPEN_ALWAYS,
				0,
				0
			);
		}
		else {
			hFile = CreateFile(writePath,
				GENERIC_WRITE,
				0,
				0,
				CREATE_ALWAYS,
				0,
				0
			);
		}
	}

	if (hFile == INVALID_HANDLE_VALUE) {
		MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
		hFile = NULL;
		return;
	}

	WriteFile(hFile,
		&val,
		1,
		&dwNumberOfBytesWritten,
		NULL);
}


