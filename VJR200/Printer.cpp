// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class Printer
// プリンタ基底クラス
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "VJR200.h"
#include "JRSystem.h"
#include "Printer.h"

extern JRSystem sys;

const float Printer::LEDON_TIME = 0.5f;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// プリンタデータアイコンを点灯
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Printer::DataActive()
{
	DrawPrinterIcon(true);
	bCountOn = true;
	count = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// プリンタデータアイコンの点灯時間を管理するカウンタ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Printer::CountUp()
{
	if (!bCountOn) return;

	count += (float)1 / g_refRate;
	if (count > LEDON_TIME) {
		DrawPrinterIcon(false);
		Finish(false);
		count = 0;
		bCountOn = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// メニューバー上にプリンタデータアイコンを表示
//
// 引数　bActive = true ならデータ受信状態
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Printer::DrawPrinterIcon(bool bActive)
{
	if (!sys.pCrtc->IsFullScreen() && g_bPrinterLed) {
		int bmpSize = 16;
		int cyTitleBar = GetSystemMetrics(SM_CYCAPTION);
		int cxFrame = GetSystemMetrics(SM_CXFRAME);
		int cyFrame = GetSystemMetrics(SM_CYFRAME);
		RECT rcWnd;
		GetWindowRect(g_hMainWnd, &rcWnd);

		HDC	hdc = GetWindowDC(g_hMainWnd);
		HDC hdcMem = CreateCompatibleDC(hdc);
		HGDIOBJ	hold;
		if (bActive)
			hold = SelectObject(hdcMem, bmpLedActive);
		else
			hold = SelectObject(hdcMem, bmpLedInactive);

		BitBlt(hdc, rcWnd.right - rcWnd.left - cxFrame * 2 - bmpSize - 1, cyTitleBar + cyFrame * 2 + 1, bmpSize, bmpSize, hdcMem, 0, 0, SRCCOPY);
		SelectObject(hdcMem, hold);
		DeleteDC(hdcMem);
		ReleaseDC(g_hMainWnd, hdc);
	}
}

