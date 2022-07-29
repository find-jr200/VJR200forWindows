// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __CRTC_H__
#define __CRTC_H__
#include <cstdint>
#ifndef _ANDROID
#include <atlbase.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wincodecsdk.h>
#endif
#include "Address.h"

extern const int BITMAP_W;
extern const int BITMAP_H;

class Crtc
{
public:
	Crtc();
	~Crtc();
	bool Init();
	uint8_t Read(int r);
	void Write(int r, uint8_t b);

	void Resize(bool bFullScreen);
	void TickCounter(int cycles);
	bool IsFullScreen() { return bFullScreen; };
#ifndef _ANDROID
	HRESULT OnRender();
	void DiscardDeviceResources();
#else
	void OnRender(uint32_t*  s);
#endif
	template <class Archive>
	void serialize(Archive & ar, std::uint32_t const version);

protected:
	static const uint16_t PCGRAM1 = 0xc000;
	static const uint16_t TVRAM = 0xc100;
	static const uint16_t PCGRAM2 = 0xc400;
	static const uint16_t AVRAM = 0xc500;
	static const uint16_t FRAM = 0xd000;

	static const int COUNTER_WIDTH = 50;
	static const int CMTSTAT_WIDTH = 40;

	static const uint32_t JrColor[8];

	uint8_t borderColor = 0;
	uint32_t* pixelData = nullptr;
	void SetPixelData();
#ifndef _ANDROID
	static const wchar_t msc_fontName[];
	static const FLOAT msc_fontSize;

	IDWriteTextFormat* pTextFormatL = NULL;
	IDWriteTextFormat* pTextFormatR = NULL;
	ID2D1HwndRenderTarget* pHwndRT = NULL;
	ID2D1Bitmap* pBitmap = NULL;
	ID2D1SolidColorBrush *pGrayBrush = NULL;
	ID2D1SolidColorBrush *pBlackBrush = NULL;
	ID2D1Bitmap *pFd1active = NULL, *pFd1Inactive = NULL, *pFd2active = NULL, *pFd2Inactive = NULL;

	HRESULT CreateDeviceResources();
	HRESULT CreateDeviceIndependentResources();
	HRESULT ConvertToD2DBitmap(ID2D1RenderTarget * pRenderTarget, IWICImagingFactory * pIWICFactory, UINT resourceId, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap ** pBitmap);


#endif

	bool bFullScreen = false;
	uint8_t getVal = 0;
};

template<class Archive>
inline void Crtc::serialize(Archive & ar, std::uint32_t const version)
{
	ar(borderColor, bFullScreen, getVal);
}

#endif
