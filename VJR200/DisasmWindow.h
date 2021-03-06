// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __DISASMWINDOW_H__
#define __DISASMWINDOW_H__
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

LRESULT CALLBACK WndProcDisasmWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct OperationSet
{
	int startAddress; // オペレーションがスタートするアドレス
	wchar_t bytesString[11]; // オペコード＋オペランドをスペースで区切った文字列
	uint8_t byteArray[20]; // オペコード＋オペランドの配列
	wchar_t nimonic[10]; // ニーモニック文字列
	wchar_t operand[20]; // オペランド文字列
};


class DisasmWindow
{
public:
	DisasmWindow();
	~DisasmWindow();
	HRESULT Init();
	HWND GetHWnd();
	void Show();
	HRESULT OnRender();
	void OnResize(
		UINT width,
		UINT height
	);
	void Close();

protected:
	HWND hwnd;
	IWICImagingFactory *pWICFactory;
	ID2D1HwndRenderTarget *pRenderTarget;
	IDWriteTextFormat *pTextFormat;
	ID2D1SolidColorBrush *pBlackBrush;
	IDWriteTextLayout* pTextLayout;

	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();
	void DiscardDeviceResources();

	void SetOperation(OperationSet* oSet);
	void MakeOperand(wchar_t* s, vector<uint8_t>& ops, int amode, int bytes);

	uint16_t curAddress /* , startAddress */;

	static const wchar_t msc_fontName[];
	static const FLOAT msc_fontSize;

	static const wchar_t *const nimonic[256];
	static const int bytes[256];
	static const int addressing[256];

	int prevAddress;
	SCROLLINFO si;

};

#endif