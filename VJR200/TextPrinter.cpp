// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class TextPrinter
// テキストプリンタ出力
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "TextPrinter.h"
#include "VJR200.h"
#include "Mn1271.h"
#include <Shlobj.h>
#include <Shlwapi.h>

TextPrinter::TextPrinter()
{
}


TextPrinter::~TextPrinter()
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
bool TextPrinter::Init()
{
	SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, NULL, myDocumentsFolder);
	_tcscpy(writePath, myDocumentsFolder);
	_tcscat(writePath, _T("\\printout.txt"));

	bmpLedActive = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PRNTXT_ACTIVE));
	bmpLedInactive = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PRNTXT_INACTIVE));

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ページ送り
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void TextPrinter::Finish(bool bManual)
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
void TextPrinter::Write(uint8_t val)
{
	DataActive();

	DWORD dwNumberOfBytesWritten;

	if (val >= 0 && val < 0xa) return;
	if (val > 0xa && val < 0x20) return;
	if (val >= 0x7f && val <= 0xa0) return;
	if (val >= 0xe0 && val <= 0xff) return;

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

	if (val == 0x0a) {
		uint8_t c = 0x0d;
		WriteFile(hFile,
			&c,
			1,
			&dwNumberOfBytesWritten,
			NULL);
		c = 0x0a;
		WriteFile(hFile,
			&c,
			1,
			&dwNumberOfBytesWritten,
			NULL);
	}
	else {
		WriteFile(hFile,
			&val,
			1,
			&dwNumberOfBytesWritten,
			NULL);
	}
}


