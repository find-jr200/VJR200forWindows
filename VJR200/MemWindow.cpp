// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class MemWindow
// メモリ内容を表示するクラス
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "VJR200.h"
#include "JRSystem.h"
#include "MemWindow.h"

static const LPCWSTR szWindowClassMem = L"Memory Window";
static const int MAXLINES = 4096;

static int lineHeight = 16;
static int dispno, pos;
static SCROLLINFO si;
static int deltaSum;

const wchar_t MemWindow::msc_fontName[] = L"Courier New";
const FLOAT MemWindow::msc_fontSize = 14;

extern JRSystem sys;

MemWindow::MemWindow(): hwnd(NULL),
pRenderTarget(NULL),
pTextFormat(NULL),
pBlackBrush(NULL),
pTextLayout(NULL)
{
}

MemWindow::~MemWindow()
{
	SafeRelease(&pTextFormat);
	SafeRelease(&pTextLayout);

	DiscardDeviceResources();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT MemWindow::Init()
{
	HRESULT hr;
	hr = CreateDeviceIndependentResources();

	if (SUCCEEDED(hr)) {
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProcMemWindow;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = g_hInst;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = szWindowClassMem;
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		RegisterClassEx(&wcex);

		RECT rect;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		hwnd = CreateWindow(
			szWindowClassMem,
			g_strTable[(int)Msg::Memory_Window],
			WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL | WS_MINIMIZEBOX,
			rect.right - 500,
			0,
			500,
			rect.bottom,
			g_hMainWnd,
			NULL,
			g_hInst,
			nullptr
		);

		hr = hwnd ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			ShowWindow(hwnd, SW_SHOWNA);
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウハンドルを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HWND MemWindow::GetHWnd()
{
	return hwnd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウ表示
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void MemWindow::Show()
{
	ShowWindow(hwnd, SW_SHOWNA);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバイス独立リソース生成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT MemWindow::CreateDeviceIndependentResources()
{
	HRESULT hr;

	// Create a DirectWrite text format object.
	hr = g_pDWriteFactory->CreateTextFormat(
		msc_fontName,
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		msc_fontSize,
		L"", //locale
		&pTextFormat
	);

	if (SUCCEEDED(hr))
	{
		// Center the text horizontally and vertically.
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバイスリソース生成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT MemWindow::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if (!pRenderTarget)
	{

		RECT rc;
		GetClientRect(hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
		);

		// Create a Direct2D render target.
		hr = g_pD2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, size),
			&pRenderTarget
		);
		if (SUCCEEDED(hr))
		{
			// Create a black brush.
			hr = pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				&pBlackBrush
			);
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirecWrite描画
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT MemWindow::OnRender()
{
	HRESULT hr;

	hr = CreateDeviceResources();

	if (SUCCEEDED(hr) && !(pRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		// Retrieve the size of the render target.
		D2D1_SIZE_F renderTargetSize = pRenderTarget->GetSize();

		pRenderTarget->BeginDraw();

		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		wchar_t  c[8000] = {};
		wcscat(c, L"     00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F\r\n-----------------------------------------------------");

		hr = g_pDWriteFactory->CreateTextLayout(
			c,
			(UINT32)wcslen(c),
			pTextFormat,
			renderTargetSize.width,
			renderTargetSize.height,
			&pTextLayout
		);
		DWRITE_TEXT_METRICS tm;
		pTextLayout->GetMetrics(&tm);
		lineHeight = (int)(tm.height / 2) + 1;
		dispno = (int)(renderTargetSize.height / lineHeight);

		D2D1_POINT_2F points;
		points.x = 5.0f;
		points.y = 0.0f;
		if (SUCCEEDED(hr))
			pRenderTarget->DrawTextLayout(points, pTextLayout, pBlackBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
		SafeRelease(&pTextLayout);

		c[0] = '\0';
		int startAddress = pos * 16;
		wchar_t tmp[90] = {};
		for (int i = 0; i < dispno - 2; ++i) {
			uint8_t adr[16];
			if (startAddress + i * 16 <= 0xfff0) {
				for (int j = 0; j < 16; ++j)
					adr[j] = sys.pAddress->ReadByteForDebug(startAddress + i * 16 + j);

				wsprintf(tmp, L"%04X:%02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
					startAddress + i * 16,
					adr[0], adr[1], adr[2], adr[3], adr[4], adr[5], adr[6], adr[7],
					adr[8], adr[9], adr[10], adr[11], adr[12], adr[13], adr[14], adr[15]);
				wcscat(c, tmp);
			}
		}

		g_pDWriteFactory->CreateTextLayout(
			c,
			(UINT32)wcslen(c),
			pTextFormat,
			renderTargetSize.width,
			renderTargetSize.height,
			&pTextLayout
		);

		points.y = 0.0f + lineHeight * 2;
		pRenderTarget->DrawTextLayout(points, pTextLayout, pBlackBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
		SafeRelease(&pTextLayout);

		hr = pRenderTarget->EndDraw();

		if (hr == D2DERR_RECREATE_TARGET)
		{
			hr = S_OK;
			DiscardDeviceResources();
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// リサイズ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void MemWindow::OnResize(UINT width, UINT height)
{
	if (pRenderTarget)
	{
		D2D1_SIZE_U size;
		size.width = width;
		size.height = height;

		pRenderTarget->Resize(size);
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウを閉じる
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void MemWindow::Close()
{
	SendMessage(hwnd, WM_CLOSE, 0, 0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバイスリソース廃棄
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void MemWindow::DiscardDeviceResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBlackBrush);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウプロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProcMemWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	bool wasHandled = false;
	RECT rect;

	switch (message) {
	case WM_SIZE:
	{
		UINT w = LOWORD(lParam);
		UINT h = HIWORD(lParam);
		g_pMemWindow->OnResize(w, h);

		GetClientRect(hWnd, &rect);
		dispno = rect.bottom / lineHeight;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = MAXLINES;
		si.nPage = dispno;
		si.nPos = pos;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		break;
	}
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_UP:
			SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
			break;
		case VK_DOWN:
			SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
			break;
		case VK_PRIOR:
			SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0);
			break;
		case VK_NEXT:
			SendMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
			break;
		}
		si.nPos = pos;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_MOUSEWHEEL:
	{
		WORD keys = GET_KEYSTATE_WPARAM(wParam);
		short delta = GET_WHEEL_DELTA_WPARAM(wParam);
		bool page = (keys & (MK_CONTROL | MK_SHIFT)) != 0 ? true : false;

		deltaSum += delta;

		if (WHEEL_DELTA <= deltaSum)
		{
			deltaSum -= WHEEL_DELTA;
			SendMessage(hWnd, WM_VSCROLL, page ? SB_PAGEUP : SB_LINEUP, 0);
		}
		else if (deltaSum <= -WHEEL_DELTA)
		{
			deltaSum += WHEEL_DELTA;
			SendMessage(hWnd, WM_VSCROLL, page ? SB_PAGEDOWN : SB_LINEDOWN, 0);
		}
		break;
	}
	case WM_VSCROLL:
		switch (LOWORD(wParam)) {
		case SB_TOP:
			pos = 0;
			break;
		case SB_LINEUP:
			if (pos > 0) pos--;
			break;
		case SB_LINEDOWN:
			if (pos <= MAXLINES - dispno + 1) pos++;
			break;
		case SB_PAGEUP:
			pos -= dispno - 2;
			if (pos < 0) pos = 0;
			break;
		case SB_PAGEDOWN:
			pos += dispno - 2;
			if (pos > MAXLINES - dispno + 2) pos = MAXLINES - dispno + 2;
			break;
		case SB_BOTTOM:
			pos = MAXLINES - dispno + 1;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			pos = HIWORD(wParam);
			break;
		}
		si.nPos = pos;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_PAINT:
	case WM_DISPLAYCHANGE:
	{
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		g_pMemWindow->OnRender();
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		g_bMemWindow = false;
		CheckMenuItem(GetMenu(g_hMainWnd), IDM_TOOL_MEMORYWINDOW, MF_UNCHECKED);
		delete g_pMemWindow;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
