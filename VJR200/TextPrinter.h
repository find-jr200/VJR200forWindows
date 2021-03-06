// license:BSD-3-Clause
// copyright-holders:FIND
#pragma once
#include "Printer.h"

class TextPrinter :	public Printer
{
public:
	TextPrinter();
	~TextPrinter();
	bool Init();
	virtual void Write(uint8_t val);
	virtual void Finish(bool bManual);

protected:
	TCHAR myDocumentsFolder[MAX_PATH];
	TCHAR writePath[MAX_PATH];
	HANDLE hFile = NULL;

};

