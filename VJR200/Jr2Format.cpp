// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class Jr2Format
// JR2ファイルフォーマットの操作全般
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <sys/stat.h>
#include <string.h>
#include "VJR200.h"
#include "Jr2Format.h"

const uint8_t Jr2Format::CHECK_ARRAY[12] = { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xcc, 0xcc, 0xcc };

CEREAL_CLASS_VERSION(Jr2Format, CEREAL_VER);

Jr2Format::Jr2Format() : pointer(this)
{
	majVer = 1;
	minVer = 0;
}


Jr2Format::~Jr2Format()
{
	if (fp != nullptr)
		fclose(fp);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Jr2Format::Init(const TCHAR * fn)
{
	_tcsncpy(fileName, fn, MAX_PATH);

	struct _stat buf;
	int st = _tstat(fileName, &buf);
	fileSize = buf.st_size;

	if (st != 0) {
		fp = _tfopen(fileName, _T("wb"));
		if (fp == nullptr)
			return false;
		fwrite(HEADER, sizeof(uint8_t), sizeof(HEADER) / sizeof(uint8_t), fp);
		fclose(fp);
		fp = nullptr;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// タイプを文字列で返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
const TCHAR * Jr2Format::GetType()
{
	return _T("JR2");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル名を返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
const TCHAR* Jr2Format::GetFileName()
{
	return fileName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1バイト書き込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Jr2Format::WriteByte(uint8_t b)
{
	bWriting = true;
	if (fp != nullptr) {
		fwrite(&b, sizeof(uint8_t), sizeof(uint8_t), fp);
		pointer.bytePointer = ftell(fp);
		pointer.bitPointer = 7;
		fileSize = pointer.bytePointer;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1ビット分のロードデータを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Jr2Format::GetLoadData()
{
	bCountStart = true;
	return lastBit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CPUの消費クロック分カウンタを進める
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Jr2Format::TickCounter(int c)
{
	if (!bCountStart) return;

	counter += c;
	if (counter >= DATA_CYCLE) {
		counter -= DATA_CYCLE;
		pointer.Next();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル位置をバイト数で返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t Jr2Format::GetPoiner()
{
	return pointer.bytePointer - DATA_BLOCK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル位置を秒数で返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t Jr2Format::GetTimeCounter()
{
	uint32_t u = pointer.bytePointer - DATA_BLOCK;
	u = u * 1668 / 1000000;

	return u;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル位置を先頭へ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Jr2Format::Top()
{
	pointer.Top();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 次のプログラムの頭出し
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Jr2Format::Next()
{
	uint32_t p1, p2, p3, fileSize;
	uint8_t d;
	int c = 0;
	bool bOpened = false;

	p1 = pointer.bytePointer;

	if (fp == nullptr) {
		fp = _tfopen(fileName, _T("rb"));
		bOpened = true;
	}
	if (fp == nullptr) {
#ifndef _ANDROID
		MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
#endif
		return;
	}
	struct _stat buf;
	_tstat(fileName, &buf);
	fileSize = buf.st_size;

	while (p1 != fileSize) {
		fseek(fp, p1, SEEK_SET);
		fread(&d, sizeof(uint8_t), 1, fp);
		if (d == 0xcc || d == 0) {
			do {
				++p1;
				fseek(fp, p1, SEEK_SET);
				fread(&d, sizeof(uint8_t), 1, fp);
			} while ((d == 0xcc || d == 0) && p1 < fileSize);
		}
		if (p1 >= fileSize) break;

		do {
			++p1;
			fseek(fp, p1, SEEK_SET);
			fread(&d, sizeof(uint8_t), 1, fp);
			if (d == 0xcc)
				++c;
			else
				c = 0;
		} while (c < 50 && p1 < fileSize);

		if (p1 >= fileSize) break;

		do {
			++p1;
			fseek(fp, p1, SEEK_SET);
			fread(&d, sizeof(uint8_t), 1, fp);
		} while ((d == 0xcc || d == 0) && p1 < fileSize);

		if (p1 >= fileSize) break;

		p2 = p1;
		do {
			--p1;
			fseek(fp, p1, SEEK_SET);
			fread(&d, 1, 1, fp);
		} while (p1 != 0 && d == 0xcc);
		++p1;
		if (p1 <= DATA_BLOCK) p1 = DATA_BLOCK;

		p3 = p2;
		p3 += 24;
		uint8_t a[12] = {};
		for (int i = 0; i < 12; ++i) {
			fseek(fp, p3 + i, SEEK_SET);
			fread(&d, sizeof(uint8_t), 1, fp);
			a[i] = d;
		}

		bool bCheckArray = true;
		for (int i = 0; i < 12; ++i) {
			if (a[i] != CHECK_ARRAY[i])
				bCheckArray = false;
		}

		if (bCheckArray) {
			pointer.bytePointer = p1;
			pointer.bitPointer = 8;
			if (bOpened) {
				fclose(fp);
				fp = nullptr;
			}
			return;
		}
	}

	pointer.bytePointer = fileSize - 1;
	if (bOpened) {
		fclose(fp);
		fp = nullptr;
	}
	return;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ひとつ前のプログラムの頭出し
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Jr2Format::Prev()
{
	uint32_t p1, p2, p3 , fileSize;
	uint8_t d;
	int c = 0;
	bool bOpened = false;

	p1 = pointer.bytePointer;
	if (p1 <= DATA_BLOCK) {
		pointer.Top();
		return;
	}

	if (fp == nullptr) {
		fp = _tfopen(fileName, _T("rb"));
		bOpened = true;
	}
	if (fp == nullptr) {
#ifndef _ANDROID
		MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
#endif
		return;
	}
	struct _stat buf;
	_tstat(fileName, &buf);
	fileSize = buf.st_size;

	while (p1 != 0) {
		do {
			--p1;
			fseek(fp, p1, SEEK_SET);
			fread(&d, sizeof(uint8_t), 1, fp);
			if (d == 0xcc)
				++c;
			else
				c = 0;
		} while (p1 != 0 && c < 50);

		if (p1 == 0) break;

		do {
			--p1;
			fseek(fp, p1, SEEK_SET);
			fread(&d, sizeof(uint8_t), 1, fp);
		} while (p1 != 0 && d == 0xcc);

		if (p1 == 0) break;
		++p1;
		p2 = p1;
		do {
			++p2;
			fseek(fp, p2, SEEK_SET);
			fread(&d, sizeof(uint8_t), 1, fp);
		} while (p2 != fileSize && d == 0xcc);

		if (p2 == fileSize) {
			pointer.bytePointer = p1;
			pointer.bitPointer = 8;
			if (bOpened) {
				fclose(fp);
				fp = nullptr;
			}
			return;
		}

		p3 = p2;
		p3 += 24;
		uint8_t a[12] = {};
		for (int i = 0; i < 12; ++i) {
			fseek(fp, p3 + i, SEEK_SET);
			fread(&d, sizeof(uint8_t), 1, fp);
			a[i] = d;
		}

		bool bCheckArray = true;
		for (int i = 0; i < 12; ++i) {
			if (a[i] != CHECK_ARRAY[i])
				bCheckArray = false;
		}

		if (bCheckArray) {
			pointer.bytePointer = p1;
			pointer.bitPointer = 8;
			if (bOpened) {
				fclose(fp);
				fp = nullptr;
			}
			return;
		}
	}
	pointer.Top();
	if (bOpened) {
		fclose(fp);
		fp = nullptr;
	}
	return;
}

void Jr2Format::FF()
{
}

void Jr2Format::Rew()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// リモート端子が録音・再生になった時の処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Jr2Format::remoteOn()
{
	if (fp == nullptr) {
        if (bReadOnly)
            fp = _tfopen(fileName, _T("rb"));
        else
            fp = _tfopen(fileName, _T("a+b"));
    }

	if (fp == nullptr) {
#ifndef _ANDROID
		MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
#endif
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// リモート端子が停止になった時の処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Jr2Format::remoteOff()
{
	if (fp != nullptr) {
		if (bWriting && g_bCmtAddBlank) {
			uint8_t b = 0;
			for (int i = 0; i < BLANK_LENGTH; ++i)
				fwrite(&b, sizeof(uint8_t), sizeof(uint8_t), fp);
			bWriting = false;
		}
		fclose(fp);
		fp = nullptr;
	}
	bCountStart = false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Read ony 設定
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Jr2Format::SetReadOnly(bool b)
{
	bReadOnly = b;
}