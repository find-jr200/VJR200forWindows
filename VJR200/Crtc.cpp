// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class Crtc
// CRTコントローラ　Dicrect2Dデバイスを作成し描画
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#define NOMINMAX
#ifndef _ANDROID
#include "Commdlg.h"
#endif
#include "JRSystem.h"
#include "Crtc.h"
#include "VJR200.h"

extern JRSystem sys;

CEREAL_CLASS_VERSION(Crtc, CEREAL_VER);

#ifndef _ANDROID
const wchar_t Crtc::msc_fontName[] = L"Verdana";
const FLOAT Crtc::msc_fontSize = 12.f;

const uint32_t Crtc::JrColor[8] = {	//ARGB
	0xff000000, // black
	0xff0000ff, // blue
	0xffff0000, // red
	0xffff00ff, // magenta
	0xff00ff00, // green
	0xff00ffff, // cyan
	0xffffff00, // yellow
	0xffffffff }; // white
#else
const uint32_t Crtc::JrColor[8] = {// Android
        0xff000000, // black
        0xffff0000, // blue
        0xff0000ff, // red
        0xffff00ff, // magenta
        0xff00ff00, // green
        0xffffff00, // cyan
        0xff00ffff, // yellow
        0xffffffff // white
};
#endif


Crtc::Crtc()
{}

Crtc::~Crtc()
{
#ifndef _ANDROID
    if (pixelData != nullptr) delete[] pixelData;
	DiscardDeviceResources();
	SafeRelease(&pTextFormatR);
	SafeRelease(&pTextFormatL);
#endif
}

#ifndef _ANDROID
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバイス独立リソース生成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT Crtc::CreateDeviceIndependentResources()
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
		&pTextFormatL
	);

	if (SUCCEEDED(hr))
	{
		// Create a DirectWrite text format object.
		hr = g_pDWriteFactory->CreateTextFormat(
			msc_fontName,
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			msc_fontSize,
			L"", //locale
			&pTextFormatR
		);
	}

	if (SUCCEEDED(hr))
	{
		// Center the text horizontally and vertically.
		pTextFormatL->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pTextFormatL->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

	if (SUCCEEDED(hr))
	{
		// Center the text horizontally and vertically.
		pTextFormatR->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
		pTextFormatR->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}
	return hr;
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//　	初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Crtc::Init()
{
#ifndef _ANDROID
    // Direct2D初期化
	HRESULT hr;
	hr = CreateDeviceIndependentResources();
	if (FAILED(hr))
		return false;
#endif
    pixelData = new uint32_t[BITMAP_H * BITMAP_W];
    if (pixelData == nullptr)
        return false;

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//　	読み出し
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Crtc::Read(int r)
{
    return getVal;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//　	書き込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Crtc::Write(int r, uint8_t b)
{
    borderColor =  7 & b;
}

#ifndef _ANDROID
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Direct2Dデバイス作成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT Crtc::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if (!pHwndRT)
	{
		RECT rc;
		GetClientRect(g_hMainWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
		);

		// Create a Direct2D render target.
		hr = g_pD2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(g_hMainWnd, size),
			&pHwndRT
		);

		ZeroMemory(pixelData, sizeof(uint32_t)* BITMAP_W*BITMAP_H);

		if (SUCCEEDED(hr))
		{
			D2D1_SIZE_U size = D2D1::SizeU(BITMAP_W, BITMAP_H);
			D2D1_PIXEL_FORMAT f = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
			hr = pHwndRT->CreateBitmap(size, pixelData, BITMAP_W * sizeof(uint32_t), D2D1::BitmapProperties(f), &pBitmap);

			// Create a black brush.
			hr = pHwndRT->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::LightGray),
				&pGrayBrush
			);
			hr = pHwndRT->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				&pBlackBrush
			);
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバイスリソース削除
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Crtc::DiscardDeviceResources()
{
	SafeRelease(&pBitmap);
	SafeRelease(&pGrayBrush);
	SafeRelease(&pBlackBrush);
	SafeRelease(&pHwndRT);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウサイズ変更
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Crtc::Resize(bool bParamFullScreen)
{
#ifndef _ANDROID
    DiscardDeviceResources();
#endif
    bFullScreen = bParamFullScreen;
}

#ifndef _ANDROID
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Direct2D描画
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT Crtc::OnRender()
{
	HRESULT hr = S_OK;
	int statusbarHeight = g_bStatusBar ? STATUSBAR_HEIGHT: 0;

	hr = CreateDeviceResources();
	if (SUCCEEDED(hr))
	{
		pHwndRT->BeginDraw();
		pHwndRT->SetTransform(D2D1::Matrix3x2F::Identity());
		pHwndRT->Clear(D2D1::ColorF(D2D1::ColorF::Black));

		SetPixelData(); // ここでJR-200の表示作成
		hr = pBitmap->CopyFromMemory(NULL, pixelData, BITMAP_W * sizeof(uint32_t));

		D2D1_SIZE_F rtSize = pHwndRT->GetSize();
		D2D1_RECT_F dst;

		if (bFullScreen)
		{
			int dspW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			int dspH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
			float jrAspect = (float)BITMAP_W / BITMAP_H;
			float dspAspect = (float)dspW / dspH;
			if (dspAspect > jrAspect)
			{
				// 横長
				rtSize.height = (float)dspH;
				if (g_bSquarePixel)
					rtSize.width = dspH * jrAspect;
				else
					rtSize.width = dspH * jrAspect * REAL_WRATIO;
				float offset = (dspW - rtSize.width) / (float)2;
				dst = D2D1::RectF(offset, 0, rtSize.width + offset, rtSize.height);
			}
			else {
				// 縦長
				if (g_bSquarePixel)
					rtSize.height = dspW / jrAspect;
				else
					rtSize.height = dspW / jrAspect * REAL_WRATIO;
				rtSize.width = (float)dspW;
				float offset = (dspH - rtSize.height) / (float)2;
				dst = D2D1::RectF(0, offset, rtSize.width, rtSize.height + offset);
			}
		}
		else {
			dst = D2D1::RectF(0, 0, rtSize.width, rtSize.height - statusbarHeight);
		}

		if (g_bSmoothing) {
			pHwndRT->DrawBitmap(pBitmap, &dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
		}
		else {
			pHwndRT->DrawBitmap(pBitmap, &dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
		}

		if (!bFullScreen && statusbarHeight != 0)
		{
			RECT rect;
			GetClientRect(g_hMainWnd, &rect);

			D2D1_RECT_F statusBarRect = D2D1::RectF(0, rtSize.height - statusbarHeight, rtSize.width, rtSize.height);
			D2D1_RECT_F fileNameRect = D2D1::RectF(0, rtSize.height - statusbarHeight, rtSize.width - COUNTER_WIDTH - CMTSTAT_WIDTH, rtSize.height);
			D2D1_RECT_F counterRect = D2D1::RectF(rtSize.width - COUNTER_WIDTH, rtSize.height - statusbarHeight, rtSize.width, rtSize.height);
			D2D1_RECT_F cmtRect = D2D1::RectF(rtSize.width - COUNTER_WIDTH - CMTSTAT_WIDTH, rtSize.height - statusbarHeight, rtSize.width - COUNTER_WIDTH, rtSize.height);

			pHwndRT->FillRectangle(statusBarRect, pGrayBrush);
			wchar_t buf[MAX_PATH] = {};
			if (g_pTapeFormat != nullptr)
				GetFileTitle(g_pTapeFormat->GetFileName(), buf, MAX_PATH);

			// ファイル名
			pHwndRT->DrawText(
				buf,
				(uint32_t)_tcslen(buf),
				pTextFormatL,
				fileNameRect,
				pBlackBrush
			);

			if (g_pTapeFormat != nullptr) {
				// タイムカウンタ
				uint32_t u = g_pTapeFormat->GetTimeCounter();
				unsigned int min = u / 60;
				u -= min * 60;
				wsprintf(buf, _T("%u:%#02u"), min, u);
				//wsprintf(buf, _T("%u"), g_pTapeFormat->GetPoiner());
				pHwndRT->DrawText(
					buf,
					(uint32_t)_tcslen(buf),
					pTextFormatR,
					counterRect,
					pBlackBrush
				);

				// CMTステータス
				wsprintf(buf, _T("%s:%s%s"), sys.pMn1271->GetRemoteStatus() ? _T("ON") : _T("OFF"),
					sys.pMn1271->GetReadStatus() ? _T("R") : _T(" "),
					sys.pMn1271->GetWriteStatus() ? _T("W") : _T(" "));
				pHwndRT->DrawText(
					buf,
					(uint32_t)_tcslen(buf),
					pTextFormatR,
					cmtRect,
					pBlackBrush
				);
			}
		}

		hr = pHwndRT->EndDraw();

		if (hr == D2DERR_RECREATE_TARGET)
		{
			hr = S_OK;
			DiscardDeviceResources();
		}
	}

	return hr;
}
#else // Android
void Crtc::OnRender(uint32_t*  s)
{
    pixelData = s;
    SetPixelData();
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 読み出し値設定
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Crtc::TickCounter(int cycles)
{
    g_vcyncCounter += cycles;
    int offset = (int)(2 * g_vcyncCounter / 3.f);

	while(offset >= 768)
		offset -= 768;
	
	getVal = sys.pAddress->ReadByte(AVRAM + offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JR-200描画
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Crtc::SetPixelData()
{
    for (int i = 0; i < BITMAP_H * BITMAP_W; ++i)
        pixelData[i] = JrColor[borderColor];

    for (int x = 0; x < 32; ++x) {
        for (int y = 0; y < 24; ++y) {
            uint8_t c = sys.pAddress->ReadByte(TVRAM + x + (y * 32));
            uint8_t a = sys.pAddress->ReadByte(AVRAM + x + (y * 32));

            int mode = a & 0xc0;
            a &= 0x3f;
            switch (mode)
            {
                case 0: // Normal Character
                {
                    int fColor = 7 & a;
                    int bColor = (a & 0x38) >> 3;
                    for (int i = 0; i < 8; ++i) {
                        uint8_t fontFace = sys.pAddress->ReadByte(FRAM + c * 8 + i);
                        for (int j = 0; j < 8; ++j) {
                            int k = fontFace & (0x80 >> j);
                            if (k == 0)
                                pixelData[(x * 8 + 32) + ((y * 8 + 16) * 320) + j + (i * 320)] = JrColor[bColor];
                            else
                                pixelData[(x * 8 + 32) + ((y * 8 + 16) * 320) + j + (i * 320)] = JrColor[fColor];
                        }
                    }
                    break;
                }
                case 0x40: // User Character
                {
                    int fColor = 7 & a;
                    int bColor = (a & 0x38) >> 3;

                    for (int i = 0; i < 8; ++i) {
                        uint8_t fontFace = sys.pAddress->ReadByte(PCGRAM1 + c * 8 + i);
                        for (int j = 0; j < 8; ++j) {
                            int k = fontFace & (0x80 >> j);
                            if (k == 0)
                                pixelData[(x * 8 + 32) + ((y * 8 + 16) * 320) + j + (i * 320)] = JrColor[bColor];
                            else
                                pixelData[(x * 8 + 32) + ((y * 8 + 16) * 320) + j + (i * 320)] = JrColor[fColor];
                        }
                    }
                    break;
                }
                case 0x80: // Semi Graphics
                case 0xc0:
                {
                    uint8_t semigra[4];
                    semigra[0] = (c & 0x38) >> 3; // 右上
                    semigra[1] = c & 0x07; // 左上
                    semigra[2] = (a & 0x38) >> 3; // 右下
                    semigra[3] = a & 0x07; // 左下
                    for (int i = 0; i < 4; ++i) {
                        for (int j = 0; j < 4; ++j) {
                            pixelData[(x * 8 + 32) + ((y * 8 + 16) * 320) + j + (i * 320)] = JrColor[semigra[1]];
                            pixelData[(x * 8 + 32) + ((y * 8 + 16) * 320) + j + 4 + (i * 320)] = JrColor[semigra[0]];
                            pixelData[(x * 8 + 32) + ((y * 8 + 16) * 320) + j + ((i + 4) * 320)] = JrColor[semigra[3]];
                            pixelData[(x * 8 + 32) + ((y * 8 + 16) * 320) + j + 4 + ((i + 4) * 320)] = JrColor[semigra[2]];
                        }
                    }
                    break;
                }
            }
        }
    }
}

