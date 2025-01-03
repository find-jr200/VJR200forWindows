// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __JR2FORMAT_H__
#define __JR2FORMAT_H__
#include <cstdint>
#include "ITapeFormat.h"
#include <cereal/cereal.hpp>

class Jr2Format : public ITapeFormat
{
public:
	const uint8_t HEADER[16] = { 0x4a, 0x52, 0x32, 0, 1, 0, 0x10, 0,
								 0, 0, 0, 0, 0, 0, 0, 0 };

	Jr2Format();
	virtual ~Jr2Format();
	virtual bool Init(const TCHAR * in);
	virtual const TCHAR* GetType();
	virtual const TCHAR* GetFileName();
	virtual void WriteByte(uint8_t b);
	virtual uint8_t GetLoadData();
	virtual void TickCounter(int c);
	virtual uint32_t GetPoiner();
	virtual void remoteOn();
	virtual void remoteOff();
	virtual uint32_t GetTimeCounter();
	virtual void Top();
	virtual void Next();
	virtual void Prev();
	virtual void FF();
	virtual void Rew();
    virtual void SetReadOnly(bool b);

	template<class Archive>
	void serialize(Archive & ar, std::uint32_t const version);
protected:
	static const uint8_t CHECK_ARRAY[12];
	static const int DATA_BLOCK = 16;
	static const int DATA_CYCLE = 279; // 1データあたりのCPUサイクル数
	static const int BLANK_LENGTH = 500;

	TCHAR fileName[MAX_PATH];
	unsigned int majVer;
	unsigned int minVer;
	int counter = 0;
	uint8_t lastByte = 0;
	uint8_t lastBit = 0;
	uint32_t fileSize = 0;

	FILE *fp = nullptr;
	bool bCountStart = false;
	bool bWriting = false;
    bool bReadOnly = false;

	class Pointer {
	public:
		Pointer(Jr2Format* p) {
			outer = p;
			bytePointer = DATA_BLOCK;
			bitPointer = 8;
		}

		void Top() {
			bytePointer = DATA_BLOCK;
			bitPointer = 8;
		}

		void Next(){
			if (--bitPointer < 0) {
				bitPointer = 7;
				if ((bytePointer + 1) <= outer->fileSize) {
					++bytePointer;
					uint8_t b;
					if (outer->fp == nullptr) {
                        if (outer->bReadOnly)
                            outer->fp = _tfopen(outer->fileName, _T("rb"));
                        else
                            outer->fp = _tfopen(outer->fileName, _T("a+b"));
                    }

					if (outer->fp == nullptr) {
#ifndef _ANDROID
						MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
#endif
						return;
					}
					fseek(outer->fp, (long)bytePointer, SEEK_SET);
					fread(&b, sizeof(uint8_t), 1, outer->fp);
					outer->lastByte = b;
				}
				else {
					outer->lastByte = 0;
					outer->lastBit = 0;
				}
			}
			outer->lastBit = (outer->lastByte >> (7 - bitPointer)) & 1;
		}

		uint32_t bytePointer;
		int bitPointer;
		Jr2Format* outer;
	} pointer;

};

template<class Archive>
inline void Jr2Format::serialize(Archive & ar, std::uint32_t const version)
{
	switch (version) {
		case 1:
			ar(majVer, minVer, counter, lastByte, lastBit);
			ar(bCountStart, bWriting);
			ar(pointer.bytePointer, pointer.bitPointer);
			break;
		case 2:
			ar(majVer, minVer, counter, lastByte, lastBit);
			ar(bCountStart, bWriting, bReadOnly);
			ar(pointer.bytePointer, pointer.bitPointer);
			break;
		default:
			break;
	}
}


#endif