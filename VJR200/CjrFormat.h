// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __CJRFORMAT_H__
#define __CJRFORMAT_H__
#ifndef _ANDROID
#include "stdafx.h"
#include <stdio.h>
#endif
#include <vector>
#include <cstdint>
#include "ITapeFormat.h"

using namespace std;

enum WriteType { CJR, BIN, S };

class CjrFormat : public ITapeFormat
{
public:
	CjrFormat();
	virtual ~CjrFormat();

	void SetCjrData(uint8_t* d, int size);
	uint8_t* GetCjrDataArray(char* jrFileName, int startAddress, bool basic);
	uint8_t* GetCjrDataArray();
	uint8_t* GetBinDataArray();
	int GetBinSize();
	int GetCjrSize();
	bool isBasic();
	int GetStartAddress();
	bool SetLoadDataBlock(int fileSize);
	int GetLoadData(uint8_t* data);
	uint8_t* MemToCjrBasic(char* jrFileName);
	uint8_t * MemToCjrBin(char * jrFileName, int start, int end);
	void AdjustBaudRate(int d);

	virtual bool Init(const TCHAR* fileName);
	virtual const TCHAR* GetType();
	virtual const TCHAR* GetFileName();
	virtual void WriteByte(uint8_t b);
	virtual uint8_t GetLoadData();
	virtual void TickCounter(int c);
	virtual uint32_t GetPoiner();
	virtual uint32_t GetTimeCounter();
	virtual void remoteOn();
	virtual void remoteOff();
	virtual void Top();
	virtual void Next();
	virtual void Prev();
	virtual void FF();
	virtual void Rew();
	virtual void SetReadOnly(bool b);

	template<class Archive>
	void serialize(Archive & ar, std::uint32_t const version);

protected:
	static const int DATA_CYCLE = 280; // 1データあたりのCPUサイクル数
	static const int ALLOC_SIZE = 5000000; // 作成した波形データを格納する領域サイズ

	vector<uint8_t> cjrData;
	vector<uint8_t> binData;
	int startAddress = 0;

	vector<vector<uint8_t>*> loadDataBlock; // JRにロードするためにCJRをブロックごとに分割して格納
	vector<vector<uint8_t>*>::iterator itBlock; // ロード時に使うイテレータ
	vector<uint8_t> leaderData; // ロード時に使うリーダー部分
	vector<uint8_t> intermData; // インターミッション時に使うリーダー部分

	TCHAR fileName[MAX_PATH];
	uint32_t fileSize = 0;
	FILE *fp = nullptr;
	uint8_t* pLoadData = nullptr;
	unsigned int loadDataSize = 0;

	int counter = 0;
	uint32_t pointer = 0;
	bool bCountStart = false;
	bool bReadOnly = false;
};

template<class Archive>
inline void CjrFormat::serialize(Archive & ar, std::uint32_t const version)
{
	ar(counter, pointer, bCountStart, startAddress);
}

#endif