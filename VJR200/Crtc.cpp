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

		// FDD アイコン作成
		if (SUCCEEDED(hr))
		{
			hr = ConvertToD2DBitmap(
				pHwndRT,
				pWICImagingFactory,
				IDB_FDD1_ACTIVE,
				22,
				16,
				&pFd1active
			);
		}

		if (SUCCEEDED(hr))
		{
			hr = ConvertToD2DBitmap(
				pHwndRT,
				pWICImagingFactory,
				IDB_FDD1_INACTIVE,
				22,
				16,
				&pFd1Inactive
			);
		}
		if (SUCCEEDED(hr))
		{
			hr = ConvertToD2DBitmap(
				pHwndRT,
				pWICImagingFactory,
				IDB_FDD2_ACTIVE,
				22,
				16,
				&pFd2active
			);
		}

		if (SUCCEEDED(hr))
		{
			hr = ConvertToD2DBitmap(
				pHwndRT,
				pWICImagingFactory,
				IDB_FDD2_INACTIVE,
				22,
				16,
				&pFd2Inactive
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
	SafeRelease(&pFd1active);
	SafeRelease(&pFd1Inactive);
	SafeRelease(&pFd2active);
	SafeRelease(&pFd2Inactive);
	SafeRelease(&pGrayBrush);
	SafeRelease(&pBlackBrush);
	SafeRelease(&pBitmap);
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
	int fddBmpSizeX = 22;
	int fddMargin = 2;

	hr = CreateDeviceResources();
	if (SUCCEEDED(hr))
	{
		pHwndRT->BeginDraw();
		pHwndRT->Clear(D2D1::ColorF(D2D1::ColorF::Black));

		SetPixelData(); // ここでJR-200の表示作成
		hr = pBitmap->CopyFromMemory(NULL, pixelData, BITMAP_W * sizeof(uint32_t));

		const D2D1_SIZE_F rtSize = pHwndRT->GetSize();
		D2D1_RECT_F dst = { 0.f, 0.f , 0.f , 0.f };

		float w_ratio = 1.f;
		if (!g_bSquarePixel)
			w_ratio = REAL_WRATIO;

		if (bFullScreen)
		{
			// フルスクリーン表示-----------------------------------------------------------------
			int dspW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			int dspH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
			float jrAspect = (float)BITMAP_W * w_ratio / BITMAP_H;
			float dspAspect = (float)dspW / dspH;

			if (dspAspect > jrAspect)
			{
				// 横長ディスプレイ
				dst.right = (float)dspH;
				dst.bottom = dspH / jrAspect;
				
				if (g_rotate == 90 || g_rotate == 270) {
					// 縦表示
					float offset_x = (dspW - dst.right) / 2.f;
					float offset_y = (dspH - dst.bottom) / 2.f;
					dst.left += offset_x;
					dst.right += offset_x;
					dst.top += offset_y;
					dst.bottom += offset_y;
				}
				else {
					// 横表示
					float offset = (dspW - dspH * jrAspect) / 2.f;
					dst = D2D1::RectF(offset, 0.f, dspH * jrAspect + offset, (float)dspH);
				}
			}
			else {
				// 縦長ディスプレイ
				dst.bottom = (float)dspW;

				dst.right = dspW * jrAspect;

				if (g_rotate == 90 || g_rotate == 270) {
					// 縦表示
					float offset_x = (dspW - dst.right) / 2.f;
					float offset_y = (dspH - dst.bottom) / 2.f;
					dst.left += offset_x;
					dst.right += offset_x;
					dst.top += offset_y;
					dst.bottom += offset_y;
				}
				else {
					// 横表示
					float offset = (dspH - dspW / jrAspect) / 2.f;
					dst = D2D1::RectF(0.f, offset, (float)dspW, dspW / jrAspect + offset);
				}
			}

			// 回転表示指定
			if (g_rotate == 0) {
				pHwndRT->SetTransform(D2D1::Matrix3x2F::Identity());
			}
			else {
				pHwndRT->SetTransform(D2D1::Matrix3x2F::Rotation(
					(float)g_rotate,
					D2D1::Point2F(dspW / 2.f, dspH / 2.f)
				));
			}

			// JR-200画面描画
			int interpolation = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
			if (g_bSmoothing) {
				interpolation = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
			}

			pHwndRT->DrawBitmap(pBitmap, &dst, 1.0f, (D2D1_BITMAP_INTERPOLATION_MODE)interpolation, NULL);
		}
		else {
			// Window表示----------------------------------------------------------------------------------------
			dst = D2D1::RectF(0.f, 0.f, (float)BITMAP_W * g_viewScale * w_ratio, (float)BITMAP_H * g_viewScale);

			// 回転表示指定
			if (g_rotate == 0) {
				pHwndRT->SetTransform(D2D1::Matrix3x2F::Identity());
			}
			else {
				pHwndRT->SetTransform(D2D1::Matrix3x2F::Rotation(
					(float)g_rotate,
					D2D1::Point2F(dst.right / 2, dst.bottom / 2)
				));
			}

			// JR-200画面描画
			int interpolation = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
			if (g_bSmoothing) {
				interpolation = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
			}

			switch (g_rotate) {
			case 0:
			{
				pHwndRT->DrawBitmap(pBitmap, &dst, 1.0f, (D2D1_BITMAP_INTERPOLATION_MODE)interpolation, NULL);
				break;
			}
			case 90:
			{
				int dif = (int)((dst.right - dst.bottom) / 2);
				dst.right += dif; dst.left += dif; dst.top += dif; dst.bottom += dif;
				pHwndRT->DrawBitmap(pBitmap, &dst, 1.0f, (D2D1_BITMAP_INTERPOLATION_MODE)interpolation, NULL);
				break;
			}
			case 180:
			{
				pHwndRT->DrawBitmap(pBitmap, &dst, 1.0f, (D2D1_BITMAP_INTERPOLATION_MODE)interpolation, NULL);
				break;
			}
			case 270:
			{
				int dif = (int)((dst.right - dst.bottom) / 2);
				dst.right -= dif; dst.left -= dif; dst.top -= dif; dst.bottom -= dif;
				pHwndRT->DrawBitmap(pBitmap, &dst, 1.0f, (D2D1_BITMAP_INTERPOLATION_MODE)interpolation, NULL);
				break;
			}
			default:
				pHwndRT->DrawBitmap(pBitmap, &dst, 1.0f, (D2D1_BITMAP_INTERPOLATION_MODE)interpolation, NULL);
				break;
			}
		}


		// 回転を元に戻す
		pHwndRT->SetTransform(D2D1::Matrix3x2F::Identity());

		if (!bFullScreen && statusbarHeight != 0)
		{
			RECT rect;
			GetClientRect(g_hMainWnd, &rect);

			if (g_bDetachFdd || !g_bFddEnabled) {
				fddBmpSizeX = 0;
				fddMargin = 0;
			}

			D2D1_RECT_F statusBarRect = D2D1::RectF(0, rtSize.height - statusbarHeight, rtSize.width, rtSize.height);
			D2D1_RECT_F fileNameRect = D2D1::RectF(0, rtSize.height - statusbarHeight, rtSize.width - COUNTER_WIDTH - CMTSTAT_WIDTH - fddBmpSizeX * 2 - fddMargin * 2, rtSize.height);
			D2D1_RECT_F counterRect = D2D1::RectF(rtSize.width - COUNTER_WIDTH - fddBmpSizeX * 2, rtSize.height - statusbarHeight, rtSize.width - fddBmpSizeX * 2 - fddMargin * 2, rtSize.height);
			D2D1_RECT_F cmtRect = D2D1::RectF(rtSize.width - COUNTER_WIDTH - CMTSTAT_WIDTH - fddBmpSizeX * 2, rtSize.height - statusbarHeight, rtSize.width - COUNTER_WIDTH - fddBmpSizeX * 2 - fddMargin * 2, rtSize.height);

			// テープファイル名
			pHwndRT->FillRectangle(statusBarRect, pGrayBrush);
			wchar_t buf[MAX_PATH] = {};
			if (g_pTapeFormat != nullptr)
				GetFileTitle(g_pTapeFormat->GetFileName(), buf, MAX_PATH);

			// Diskイメージ名
			if (!g_bDetachFdd && g_bFddEnabled) {
				_tcscat(buf, _T(";"));
				_tcscat(buf, sys.pFddSystem->mountedFileName[0].c_str());
				_tcscat(buf, _T(";"));
				_tcscat(buf, sys.pFddSystem->mountedFileName[1].c_str());
			}

			// ファイル名描画
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

			// FDD
			if (!g_bDetachFdd && g_bFddEnabled) {
				D2D1_RECT_F fdd1Rect = D2D1::RectF(rtSize.width - fddBmpSizeX * 2 - fddMargin, rtSize.height - statusbarHeight + 2, rtSize.width - fddBmpSizeX - fddMargin, rtSize.height - 2);
				D2D1_RECT_F fdd2Rect = D2D1::RectF(rtSize.width - fddBmpSizeX - fddMargin, rtSize.height - statusbarHeight + 2, rtSize.width - fddMargin, rtSize.height - 2);

				if (sys.pFddSystem->GetFddStatus(0)) {
					pHwndRT->DrawBitmap(pFd1active, &fdd1Rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
				}
				else {
					pHwndRT->DrawBitmap(pFd1Inactive, &fdd1Rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
				}

				if (sys.pFddSystem->GetFddStatus(1)) {
					pHwndRT->DrawBitmap(pFd2active, &fdd2Rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
				}
				else {
					pHwndRT->DrawBitmap(pFd2Inactive, &fdd2Rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
				}
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

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BITMAPリソースをD2DBITMAPに変換
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT Crtc::ConvertToD2DBitmap(
	ID2D1RenderTarget *pRenderTarget,
	IWICImagingFactory *pIWICFactory,
	UINT resourceId,
	UINT destinationWidth,
	UINT destinationHeight,
	ID2D1Bitmap **pBmp
)
{
	HRESULT hr = S_OK;

	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void* pImageFile = NULL;
	DWORD imageFileSize = 0;
	IWICStream* pStream = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICBitmapDecoder* pDecoder = NULL;
	IWICFormatConverter* pConverter = NULL;


	// Locate the resource.
	imageResHandle = FindResource(g_hInst, MAKEINTRESOURCE(resourceId), RT_BITMAP);

	hr = imageResHandle ? S_OK : E_FAIL;
	if (SUCCEEDED(hr))
	{
		// Load the resource.
		imageResDataHandle = LoadResource(g_hInst, imageResHandle);

		hr = imageResDataHandle ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		// Lock it to get a system memory pointer.
		pImageFile = LockResource(imageResDataHandle);

		hr = pImageFile ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		// Calculate the size.
		imageFileSize = SizeofResource(g_hInst, imageResHandle);

		hr = imageFileSize ? S_OK : E_FAIL;

	}

	BITMAPFILEHEADER header;
	header.bfType = 0x4D42; // 'BM'
	header.bfSize = imageFileSize + 14; // resource data size + 14 bytes header
	header.bfReserved1 = 0;
	header.bfReserved2 = 0;
	header.bfOffBits = 14 + 40; // details in bitmap format

	BYTE* buffer = new BYTE[header.bfSize];

	if (SUCCEEDED(hr))
	{
		// Create a WIC stream to map onto the memory.
		hr = pWICImagingFactory->CreateStream(&pStream);
	}

	memcpy(buffer, &header, 14);
	memcpy(buffer + 14, pImageFile, imageFileSize);

	if (SUCCEEDED(hr))
	{
		hr = pStream->InitializeFromMemory(
			reinterpret_cast<BYTE*>(buffer),
			header.bfSize
		);
	}

	if (SUCCEEDED(hr))
	{
		// Create a decoder for the stream.
		hr = pWICImagingFactory->CreateDecoderFromStream(
			pStream,
			NULL,
			WICDecodeMetadataCacheOnLoad,
			&pDecoder
		);
	}

	// Retrieve the first frame of the image from the decoder
	if (SUCCEEDED(hr))
	{
		hr = pDecoder->GetFrame(0, &pSource);
	}


	if (SUCCEEDED(hr))
	{
		// Convert the image format to 32bppPBGRA
		// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
		hr = pWICImagingFactory->CreateFormatConverter(&pConverter);
	}

	if (SUCCEEDED(hr))
	{
		hr = pConverter->Initialize(
			pSource,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeMedianCut
		);
	}

	if (SUCCEEDED(hr))
	{
		//create a Direct2D bitmap from the WIC bitmap.
		hr = pHwndRT->CreateBitmapFromWicBitmap(
			pConverter,
			NULL,
			pBmp
		);

	}

	delete[] buffer;
	SafeRelease(&pDecoder);
	SafeRelease(&pSource);
	SafeRelease(&pStream);
	SafeRelease(&pConverter);

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
