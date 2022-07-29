// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class ImagePrinter
// PNGプリンタ出力
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <Unknwn.h>  
#include <gdiplus.h>
#include <crtdbg.h>
#include <Shlobj.h>
#include "ImagePrinter.h"
#include "VJR200.h"
#include "JRSystem.h"

extern JRSystem sys;

using namespace Gdiplus;

ImagePrinter::ImagePrinter() 
{
}

ImagePrinter::~ImagePrinter()
{
	if (pBits != nullptr) {
		delete[] pBits;
		pBits = nullptr;
	}

	DeleteObject(bmpLedActive);
	DeleteObject(bmpLedInactive);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool ImagePrinter::Init()
{
	bmpLedActive = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PRNPNG_ACTIVE));
	bmpLedInactive = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PRNPNG_INACTIVE));

	return Init(WIDTH, HEIGHT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool ImagePrinter::Init(int w, int h)
{
	myPicturesFolder[0] = '\0';
	writePath[0] = '\0';
	SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, NULL, myPicturesFolder);
	
	charW = w; charH = h;
	curX = pageMargin, curY = pageMargin;

	dotW = pageMargin * 2 + charW * CHAR_SIZE;
	int m = dotW % 8;
	dotW += m;

	bytesW = dotW / 8;
	m = bytesW % 4;
	bytesW += m;
	dotH = pageMargin * 2 + charH * CHAR_SIZE;

	bitsSize = bytesW * dotH;
	pBits = new BYTE[bitsSize];

	pFontData = sys.pMn1544->fontData;

	if (pBits != nullptr) {
		memset(pBits, 0xff, bytesW * dotH);
		return true;
	} else {
		return false;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 新規ページ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void ImagePrinter::SetNewPage()
{
	memset(pBits, 0xff, bitsSize);
	curX = pageMargin; curY = pageMargin;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1byte書込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void ImagePrinter::Write(uint8_t val)
{
	DataActive();

	if (val == 0x1b) { // Esc
		bEsc = true;
		count = 0;
	}
	else {
		if (bEsc) { // ESC コマンド処理
			switch (val) {
			case 0x02:
				// 謎コマンド（行間スペースを初期値に？）
				cmd[0] = val;
				len = 1;
				bEsc = false;
				bCmd = false;
				rowMargin = ROW_MARGIN;
				break;
			case 0x41:
				// 行間スペースをゼロにしとく
				cmd[0] = val;
				bCmd = true;
				len = 2;
				rowMargin = 0;
				break;
			case 0x4b:
				// グラフィック出力
				cmd[0] = val;
				bCmd = true;
				len = 3;
				break;
			case 0x4e:
				// ミシン目スキップ（無視）
				cmd[0] = val;
				bCmd = true;
				len = 2;
				break;
			default:
				break;
			}

			if (bCmd) {
				cmd[count++] = val;
				if (--len <= 0) {
					bCmd = false;
					bEsc = false;
					count = 0;

					if (cmd[0] == 0x4b) {
						bGraphMode = true;
						graphBytes = cmd[1] + cmd[2] * 256;
						if (curX % CHAR_SIZE != 0)
							curX += curX % CHAR_SIZE;
					}
				}
			}
		}
		else {
			// ESC コマンド以外
			if (bGraphMode) {
				// グラフィックデータ出力
				for (int i = 0; i < 8; ++i) {
					uint8_t j = val & (1 << (7 - i));
					pBits[(curY + i) * bytesW + (curX / CHAR_SIZE)] &= ~(j != 0 ? 1 << (7 - (curX % CHAR_SIZE)): 0);
				}
				++curX;

				if (--graphBytes == 0) {
					bGraphMode = false;
					if (curX % CHAR_SIZE != 0)
						curX += curX % CHAR_SIZE;
				}

				if (curX >= pageMargin + dotW) {
					curX = pageMargin;
					curY += CHAR_SIZE + rowMargin;
				}
				if (curY + CHAR_SIZE >= dotH - pageMargin) {
					SavePngFile();
					SetNewPage();
				}

			}
			else {
				// 制御コード、キャラクターコード出力
				if (val == 0xa && curX != pageMargin) {
					curX = pageMargin;
					curY += CHAR_SIZE + rowMargin;

					if (curY + CHAR_SIZE >= dotH - pageMargin) {
						SavePngFile();
						SetNewPage();
					}
				}
				else if ((val >= 0x20 && val <= 0x7e) || (val >= 0xa1 && val <= 0xdf)) {
					// キャラクタ出力
					for (int i = 0; i < 8; ++i) {
						pBits[(curY + i) * bytesW + (curX / CHAR_SIZE)] = ~pFontData[val * 8 + i];
					}

					curX += CHAR_SIZE;

					if (curX >= pageMargin + charW * CHAR_SIZE) {
						curX = pageMargin;
						curY += CHAR_SIZE + rowMargin;
					}
					if (curY + CHAR_SIZE >= dotH - pageMargin) {
						SavePngFile();
						SetNewPage();
					}
				}
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ページ送り
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void ImagePrinter::Finish(bool bManual)
{
	if (g_prAutoFeed || bManual) {
		if (curX != pageMargin || curY != pageMargin) {
			if (!SavePngFile())
				MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);

			SetNewPage();
		}
	}
	return;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// PNG保存
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool ImagePrinter::SavePngFile()
{
	HDC	hDC;
	DWORD	dwHeaderSize;
	DWORD	dwScanDataSize;
	BITMAPFILEHEADER	PrinterHeader;
	BITMAPINFOHEADER	*pBmpInfoHdr;
	BYTE	*pHeaderBuffer;
	BYTE	*pScanDataBuffer;
	BITMAP	bmp;
	int iColors = 2;
	bool ret = true;

	HBITMAP hBmp = CreateBitmap(dotW, dotH, 1, 1, pBits);
	GetObject(hBmp, sizeof(BITMAP), &bmp);

	dwHeaderSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * iColors;
	pHeaderBuffer = new BYTE[dwHeaderSize];
	memset(pHeaderBuffer, 0, dwHeaderSize);

	pBmpInfoHdr = (BITMAPINFOHEADER*)pHeaderBuffer;
	pBmpInfoHdr->biBitCount = 1;

	pBmpInfoHdr->biSize = sizeof(BITMAPINFOHEADER);
	pBmpInfoHdr->biWidth = bmp.bmWidth;
	pBmpInfoHdr->biHeight = bmp.bmHeight;
	pBmpInfoHdr->biPlanes = 1;
	pBmpInfoHdr->biCompression = BI_RGB;
	pBmpInfoHdr->biSizeImage = 0;
	pBmpInfoHdr->biXPelsPerMeter = 0;
	pBmpInfoHdr->biYPelsPerMeter = 0;
	pBmpInfoHdr->biClrUsed = 0;
	pBmpInfoHdr->biClrImportant = 0;

	hDC = GetDC(NULL);
	GetDIBits(hDC, hBmp, 0, bmp.bmHeight, NULL, (LPBITMAPINFO)pBmpInfoHdr, DIB_RGB_COLORS);
	dwScanDataSize = pBmpInfoHdr->biSizeImage;
	pScanDataBuffer = new BYTE[dwScanDataSize];
	GetDIBits(hDC, hBmp, 0, bmp.bmHeight, pScanDataBuffer, (LPBITMAPINFO)pBmpInfoHdr, DIB_RGB_COLORS);
	ReleaseDC(NULL, hDC);

	PrinterHeader.bfType = 0x4d42;
	PrinterHeader.bfReserved1 = 0;
	PrinterHeader.bfReserved2 = 0;
	PrinterHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + dwHeaderSize;
	PrinterHeader.bfSize = dwScanDataSize + PrinterHeader.bfOffBits;

	CLSID clsid;
	GetEncoderClsid(L"image/png", &clsid);

	int i = 1;
	do {
		_tcscpy(writePath, myPicturesFolder);
		TCHAR c[30];
		wsprintf(c, _T("\\JR200Print%03d.png"), i);
		_tcscat(writePath, c);
		++i;
		if (i >= 1000) {
			MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
			return false;
		}
	} while (PathFileExists(writePath) && !PathIsDirectory(writePath));

	Status status = Bitmap(hBmp, NULL).Save(writePath, &clsid, NULL);
	if (status != Status::Ok)
		ret = false;

	delete[] pScanDataBuffer;
	delete[] pHeaderBuffer;

	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// （ここでは）PNGフォーマットのCLSID取得　MSDNまんまコピー
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int ImagePrinter::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

