// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __MEMWINDOW_H__
#define __MEMWINDOW_H__
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

LRESULT CALLBACK WndProcMemWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class MemWindow
{
public:
	MemWindow();
	~MemWindow();
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
	static const wchar_t msc_fontName[];
	static const FLOAT msc_fontSize;

	HWND hwnd;
	IWICImagingFactory *pWICFactory;
	ID2D1HwndRenderTarget *pRenderTarget;
	IDWriteTextFormat *pTextFormat;
	ID2D1SolidColorBrush *pBlackBrush;
	IDWriteTextLayout* pTextLayout;

	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();
	void DiscardDeviceResources();
};

#endif