// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// interface ITapeFormat
// テープフォーマットのインターフェイス
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __ITAPEFORMAT_H__
#define __ITAPEFORMAT_H__
#ifndef _ANDROID
#include <objbase.h>
#else
#define interface struct
#endif

interface ITapeFormat
{
	virtual bool Init(const TCHAR* fileName) = 0;
	virtual const TCHAR* GetType() = 0;
	virtual const TCHAR* GetFileName() = 0;
	virtual void WriteByte(uint8_t b) = 0;
	virtual uint8_t GetLoadData() = 0;
	virtual void TickCounter(int c) = 0;
	virtual uint32_t GetPoiner() = 0;
	virtual uint32_t GetTimeCounter() = 0;
	virtual void remoteOn() = 0;
	virtual void remoteOff() = 0;
	virtual void Top() = 0;
	virtual void Next() = 0;
	virtual void Prev() = 0;
	virtual void FF() = 0;
	virtual void Rew() = 0;
	virtual void SetReadOnly(bool b) = 0;

	virtual ~ITapeFormat() {};
};

#endif