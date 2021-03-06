// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class AnalyzeWave
// JR-200が書き出すCMT波形をプログラム・データに変換するクラス
//
// このクラスはJR2Rescue用に書いたものを移植しているので、この用途には無駄が多い（でも面倒だから書きかえない）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#ifndef _ANDROID
#include <Shlobj.h>
#include <Shlwapi.h>
#endif
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include "AnalyzeWave.h"
#include "VJR200.h"
#include "CjrFormat.h"

using namespace std;

AnalyzeWave::AnalyzeWave(vector<uint8_t>* data)
{
	saveData = data;
	bparam.SetBaudRate(LOW); // 最初のブロックは600baud固定なのでlowに設定
}

AnalyzeWave::~AnalyzeWave()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  1サンプル取得（もしSAVE途中にBREAKしてデータが足りない場合は例外発生）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int AnalyzeWave::GetSample()
{
	if (convertedList.size() <= readCount)
		throw 1;
	else
		return (int)convertedList[readCount++];
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  符号反転まで読んで、1200Hzなら1、2400Hzなら0、それ以外なら-1を返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int AnalyzeWave::GetHalfSpan()
{
	int d, count = 0, retVal = -1;

	// 符号反転まで読む
	do
	{
		d = GetSample();
		wState.SetState(d);
		++count;
	} while (!wState.TurnOver);

	if (count >= param.minSpan && count <= param.maxSpan)
	{
		if (count < ((param.hiFreqSpan + param.lowFreqSpan) / 2.0f)) retVal = 0; else retVal = 1;
	}

	return retVal; // 1200Hzなら1、2400Hzなら0、それ以外なら-1
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  8ビットの配列を1バイトの数値に変換
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int AnalyzeWave::BitToByte(int* BitData)
{
	int value = 0;

	for (int i = 0; i < 8; i++)
	{
		value += BitData[i] << i;
	}
	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  JRが書き出したデータを扱いやすく変換
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void AnalyzeWave::SetConvertedList()
{
	for (int i = 0; i < (int)(saveData->size()); ++i) {
		uint8_t b = (*saveData)[i];
		for (int j = 0; j < 8; j++) {
			int8_t c = (b & 1) * 255 -128;
			convertedList.push_back(c);
			b >>= 1;
		}
	}

	/////////////////////// デバッグ用 //////////////////////////
	// この部分のコメントを外すと、デバッグモード時にconvertedListの内容をBINファイルに書き出す
//#ifdef _DEBUG
//	ofstream fout;
//	fout.open("c:\\tmp\\convertedList.bin", ios::out | ios::binary | ios::trunc);
//
//	int8_t* tmp = convertedList.data();
//	for (int i = 0; i < (int)(convertedList.size()); ++i) {
//		fout.write((char *)&convertedList[i], sizeof(uint8_t));
//	}
//	fout.close();
//#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  SAVEの結果をCJRとして書き出し
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AnalyzeWave::WriteCjrFile()
{
	FILE* fp;
	TCHAR writePath[MAX_PATH];
	TCHAR writeFileName[MAX_PATH];
	uint8_t* writeData;
	int dataSize;

#ifdef _ANDROID
    if (g_dataPath == nullptr) return false;
	_tcscpy(writePath, g_dataPath);
	_tcscpy(writeFileName, writePath);
	_tcscat(writeFileName, _T("/save.cjr"));
#else
	SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, NULL, writePath);
	_tcscpy(writeFileName, writePath);
	_tcscat(writeFileName, _T("\\save.cjr"));
#endif

	struct _stat stbuf;
	int st = _tstat(writeFileName, &stbuf);

	int i = 1;
	while (st == 0 && ((stbuf.st_mode & S_IFMT) == S_IFREG)) {
		TCHAR c[20];
#ifndef _ANDROID
		wsprintf(c, _T("\\save%d.cjr"), i);
#else
        wsprintf(c, _T("/save%d.cjr"), i);
#endif
		_tcscpy(writeFileName, writePath);
		_tcscat(writeFileName, c);
		++i;
		if (i >= 1000) {
#ifndef _ANDROID
			MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], NULL, 0);
#endif
			return false;
		}
		st = _tstat(writeFileName, &stbuf);
	}

	CjrFormat cjr;
	cjr.SetCjrData(cjrList.data(), (int)cjrList.size());
	cjr.AdjustBaudRate(0);
	dataSize = cjr.GetCjrSize();
	writeData = cjr.GetCjrDataArray();

    fp = _tfopen(writeFileName, _T("wb"));
    if (fp == nullptr)
        return false;
    fwrite(writeData, sizeof(uint8_t), dataSize / sizeof(uint8_t), fp);
    fclose(fp);
    fp = nullptr;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  JRが書き出した波形データを変換
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AnalyzeWave::AnalyzeWavData()
{
	int data = 0;
	int halfWaveCount = 0;
	int cheksumValue;
	bool retVal = false;

	SetConvertedList();

	// リーダー読み飛ばし
	while (GetHalfSpan() != 0);

	// スタートビット検出
	for (int i = 0; i < bparam.hiCycle - 1; i++)
	{
		data = GetHalfSpan();
		if (data == -1) GetHalfSpan();
	}

	blockCount = 0;

	while (true) // データの末端までループ
	{
		for (int i = 0; i < 3; i++) blockHead[i] = 0; // 一応初期化
		checkSum = 0;
		dataCount = 0;
		do // 1block分読み出しループ
		{
			lastData = 0;
			for (int octet = 0; octet < 8; octet++)  // 1byte読み出し
			{
				halfWaveCount = 0;
				for (int i = 0; i < bparam.lowCycle; i++)
				{
					int d = GetHalfSpan();
					if (d == -1) d = GetHalfSpan();
					halfWaveCount += d;
				}
				halfWaveCount = (int)round((double)(halfWaveCount / bparam.lowCycle));
				if (halfWaveCount == 0) for (int j = 0; j < bparam.lowCycle; j++) GetHalfSpan(); // 2400Hz(=0) のときは残りの周期読みだし
				bitData[octet] = halfWaveCount;
			}

			// ビット列を1バイトに変換。ブロックヘッダを保持するための処理もする
			int bd = BitToByte(bitData);
			checkSum += bd;
			cjrList.push_back((uint8_t)bd);
			lastData = bd;
			if (dataCount >= 0 && dataCount <= 2) blockHead[dataCount] = (uint8_t)bd;
			dataCount++;

			// ストップビット検出
			for (int i = 0; i < bparam.lowCycle * 3; i++) GetHalfSpan();

			// スタートビットかリーダーかの判定
			halfWaveCount = 0;
			for (int i = 0; i < bparam.hiCycle; i++)
			{
				data = GetHalfSpan();
				halfWaveCount += data;
			}
			halfWaveCount = (int)round((double)(halfWaveCount / bparam.hiCycle)); // 平均値をとってスタートビット(2400Hz = 0)かどうかを判定
		} while (halfWaveCount == 0); // スタートビットならブロック内ループ
		// ＃＃＃＃ 1ブロック終了。＃＃＃＃＃＃

		if (blockCount == 0 && cjrList.size() > 23)
		{
			cheksumValue = (int)cjrList[23];
			if (blockCount == 0 && cheksumValue == 0) bparam.SetBaudRate(HIGH); // ヘッダブロックをみてbaud rateを変更
		}
		else {
			if (blockCount == 0 && cjrList.size() <= 23)
			{
				return retVal;
			}
		}

		// フッタブロックに達したかどうか判断
		if (blockHead[0] == 2 && blockHead[1] == 0x2a && blockHead[2] == (uint8_t)0xff)
		{
			retVal = true;
			break;
		}

		// リーダー読み飛ばし
		while ((data = GetHalfSpan()) == 1);

		if (data == -1)
			break;

		// スタートビット検出
		for (int i = 0; i < bparam.hiCycle - 1; i++) GetHalfSpan();
		blockCount++;
	}

	return retVal;
}

