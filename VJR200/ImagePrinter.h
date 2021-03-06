// license:BSD-3-Clause
// copyright-holders:FIND
#pragma once
#include <cstdint>
#include "Printer.h"

class ImagePrinter : public Printer
{
public:
	ImagePrinter();
	virtual bool Init();
	virtual~ImagePrinter();
	virtual void Write(uint8_t val);
	virtual void Finish(bool bManual);
protected:
	static const int WIDTH = 80, HEIGHT = 120;

	TCHAR myPicturesFolder[MAX_PATH];
	TCHAR writePath[MAX_PATH];

	void SetNewPage();
	bool SavePngFile();

	int charW, charH;
	int dotW, dotH;
	int bytesW;
	int curX, curY;
	uint8_t* pFontData;
	BYTE* pBits = nullptr;
	int bitsSize;
	const int CHAR_SIZE = 8;
	const int ROW_MARGIN = 2;
	int pageMargin = 16;
	int rowMargin = ROW_MARGIN;
	int value;
	bool bGraphMode = false;
	int graphBytes;
	bool bEsc = false, bCmd = false;
	uint8_t cmd[3] = {};
	int count = 0;

	int len = 0;
private:
	bool Init(int w, int h);
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
};

