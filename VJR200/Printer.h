// license:BSD-3-Clause
// copyright-holders:FIND
#pragma once
#include <cstdint>

class Printer
{
public:
	Printer() {};
	virtual ~Printer() {};

	virtual bool Init() = 0;
	virtual void Finish(bool bManual) = 0;
	virtual void Write(uint8_t val) = 0;
	void DataActive();
	void DrawPrinterIcon(bool bActive);
	void CountUp();
protected:
	static const float LEDON_TIME;
	float count = 0;
	bool bCountOn = false;
	HBITMAP bmpLedActive, bmpLedInactive;
};

