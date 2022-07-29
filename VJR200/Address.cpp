// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class Address
// アドレス空間を管理するクラス
//
// 機能 : RAM内容を所持する、ROMファイルの読み込み、リード・ライト
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#ifndef _ANDROID
#include <shlobj.h>
#endif
#include "JRSystem.h"
#include "CjrFormat.h"
#include "Address.h"
#include "VJR200.h"

using namespace std;

extern JRSystem sys;

CEREAL_CLASS_VERSION(Address, CEREAL_VER);

Address::Address()
{
}

Address::~Address()
{
    if (memory != nullptr) delete[] memory;
	if (attribute != nullptr) delete[] attribute;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化、メモリ内容の設定
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Address::Init()
{
    memory = new uint8_t[MEM_SIZE];
    if (memory == nullptr)
        return false;

    attribute = new DevType[MEM_SIZE];
    if (attribute == nullptr)
        return false;

    memset(memory, 0, 0xffff);
	SetRamPattern(0, 0x8000);

	// ユーザー定義RAMのパターン
    for (int i = 0xc001; i < 0xc100; i += 2) {
        memory[i] = 0xff;
    }
    for (int i = 0xc401; i < 0xc500; i += 2) {
        memory[i] = 0xff;
    }

    for (int i = 0; i < 0x8000; ++i) {
        attribute[i] = DevType::Ram;
    }
    if (g_bRamExpand1) { // RAM拡張1
        for (int i = 0x8000; i < 0xa000; ++i) {
            attribute[i] = DevType::Ram;
        }
    }
    else {
        for (int i = 0x8000; i < 0xa000; ++i) {
            attribute[i] = DevType::None;
        }
    }
    if (g_bRamExpand2) { // RAM拡張2
        for (int i = 0xa000; i < 0xc000; ++i) {
            attribute[i] = DevType::Ram;
        }
    }
    else {
        for (int i = 0xa000; i < 0xc000; ++i) {
            attribute[i] = DevType::Rom;
        }
    }
    for (int i = 0xc000; i < 0xc800; ++i) {
        attribute[i] = DevType::Ram;
    }
    for (int i = 0xc800; i < 0xca00; ++i) {
        attribute[i] = DevType::Mn1271;
    }
    for (int i = 0xca00; i < 0xcc00; ++i) {
        attribute[i] = DevType::Crtc;
    }
    for (int i = 0xcc00; i < 0xd000; ++i) {
        attribute[i] = DevType::None;
    }
    for (int i = 0xd000; i < 0xd800; ++i) {
        attribute[i] = DevType::Ram;
    }
    for (int i = 0xd800; i < 0xe000; ++i) {
        attribute[i] = DevType::None;
    }
    for (int i = 0xe000; i < 0x10000; ++i) {
        attribute[i] = DevType::Rom;
    }

    return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// RAMパターンのセット
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Address::SetRamPattern(uint16_t sAddress, uint16_t eAddress)
{
	switch (g_ramInitPattern) {
	case 0:
		for (int i = sAddress; i < eAddress; i += 4) {
			if ((i & 0x100) == 0) {
				memory[i] = 0xff;
				memory[i + 1] = 0xff;
			}
			else {
				memory[i + 2] = 0xff;
				memory[i + 3] = 0xff;
			}
		}
		break;
	case 1:
		for (int i = sAddress; i < eAddress; i += 2) {
			if ((i & 0x80) == 0) {
				memory[i + 1] = 0xff;
			}
			else {
				memory[i] = 0xff;
			}
		}
		break;
	}
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 指定されたアドレスを1byte読む
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Address::ReadByte(uint16_t address)
{
	bool isHit = false;
	for (int i = 0; i < RW_BREAKPOINT_NUM; ++i) {
		if (g_rwBKEnableR[i]) {
			if (g_rwBreakPoint[i][1] < 0) {
				if (g_rwBreakPoint[i][0] == address) {
					isHit = true;
				}
			}
			else {
				if (g_rwBreakPoint[i][0] <= address && g_rwBreakPoint[i][1] >= address) {
					isHit = true;
				}
			}
		}
	}

	if (isHit) {
		g_debug = 0;
		TCHAR s[10];
		wsprintf(s, _T("%04X %ls"), address, _T("R"));
		_tcscpy(g_RWBreak, s);
	}

    DevType d = attribute[address];
    switch (d)
    {
        case DevType::None:
            return 0;
            break;
        case DevType::Rom:
            return memory[address];
            break;
        case DevType::Ram:
            if (address >= 0 && address <= 0xbfff) ++g_dramWait;
            return memory[address];
            break;
        case DevType::Mn1271:
        {
            int i = address - 0xc800;
            while (i >= 32)
                i -= 32;
            return sys.pMn1271->Read(i);
            break;
        }
        case DevType:: Crtc:
        {
            int i = address - 0xca00;
            return sys.pCrtc->Read(i);
            break;
        }
		case DevType::Fdd:
		{
			int i = address - 0xcc00;
			return sys.pFddSystem->Read(i);
			break;
		}
		default:
            assert(false);
            return 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 指定されたアドレスを1byte読む（デバッグ表示用）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Address::ReadByteForDebug(uint16_t address)
{
    DevType d = attribute[address];
    switch (d)
    {
        case DevType::None:
            return 0;
            break;
        case DevType::Rom:
            return memory[address];
            break;
        case DevType::Ram:
            if (address >= 0 && address <= 0xbfff) ++g_dramWait;
            return memory[address];
            break;
        case DevType::Mn1271:
        {
            int i = address - 0xc800;
            while (i >= 32)
                i -= 32;
            return sys.pMn1271->ReadForDebug(i);
            break;
        }
        case DevType::Crtc:
        {
            int i = address - 0xca00;
            return sys.pCrtc->Read(i);
            break;
        }
		case DevType::Fdd:
		{
			int i = address - 0xcc00;
			return sys.pFddSystem->ReadForDebug(i);
			break;
		}
		default:
            assert(false);
            return 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 指定されたアドレスに1byte書き込む
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Address::WriteByte(uint16_t address, uint8_t b)
{
	bool isHit = false;
	for (int i = 0; i < RW_BREAKPOINT_NUM; ++i) {
		if (g_rwBKEnableW[i]) {
			if (g_rwBreakPoint[i][1] < 0) {
				if (g_rwBreakPoint[i][0] == address) {
					isHit = true;
				}
			}
			else {
				if (g_rwBreakPoint[i][0] <= address && g_rwBreakPoint[i][1] >= address) {
					isHit = true;
				}
			}
		}
	}

	if (isHit) {
		g_debug = 0;
		TCHAR s[10];
		wsprintf(s, _T("%04X %ls"), address, _T("W"));
		_tcscpy(g_RWBreak, s);
	}

    DevType d = attribute[address];
    switch (d)
    {
        case DevType::None:
            break;
        case DevType::Rom:
            break;
        case DevType::Ram:
            if (address >= 0 && address <= 0xbfff) ++g_dramWait;
            memory[address] = b;
            break;
        case DevType::Mn1271:
        {
            int i = address - 0xc800;
            while (i >= 32)
                i -= 32;
            sys.pMn1271->Write(i, b);
            break;
        }
        case DevType::Crtc:
        {
            int i = address - 0xca00;
            sys.pCrtc->Write(i, b);
            break;
        }
		case DevType::Fdd:
		{
			int i = address - 0xcc00;
			sys.pFddSystem->Write(i, b);
			break;
		}
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ROMファイルの内容をアドレス空間に書き込む
//
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Address::LoadRomFile()
{
    FILE *fp = nullptr;
	uint8_t* buff = nullptr;
	BOOL bResult = TRUE;

    fp = _tfopen(g_pRomFile, _T("rb"));
    if (fp == nullptr) {
        _tcscpy(g_pRomFile, _T(""));
        bResult = FALSE;
    }

	if (bResult) {
		if (!(buff = new uint8_t[MEM_SIZE])) {
			fclose(fp);
			bResult = FALSE;
		}
	}

	if (bResult) {
		fread(buff, sizeof(uint8_t), MEM_SIZE, fp);
		fclose(fp);

		if (!g_bRamExpand2)
			memcpy(&memory[0xa000], buff, 8192);
		memcpy(&memory[0xe000], &buff[8192], 8192);

		// FDROM
		if (!g_bDetachFdd) {
			fp = _tfopen(g_pFdRomFile, _T("rb"));
			if (fp == nullptr) {
				_tcscpy(g_pFdRomFile, _T(""));
			}
			else {
				fread(buff, sizeof(uint8_t), MEM_SIZE, fp);
				fclose(fp);

				g_bFddEnabled = true;
				memcpy(&memory[0xd800], buff, 2048);

				memset(&memory[0x8000], 0, 16384);
				SetRamPattern(0x8000, 0xbfff);

				for (int i = 0x8000; i <= 0xbfff; ++i) {
					attribute[i] = DevType::Ram;
				}
				for (int i = 0xd800; i <= 0xdfff; ++i) {
					attribute[i] = DevType::Rom;
				}
				for (int i = 0xcc00; i <= 0xccff; ++i) {
					attribute[i] = DevType::Fdd;
				}
			}

		}

	}

	if (buff != nullptr) delete[] buff;
    return bResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// メモリ内容をファイルとして書き出し
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _ANDROID
bool Address::SaveDumpFile()
{
	FILE *fp = nullptr;
	TCHAR myDocumentsFolder[MAX_PATH];
	uint8_t* buff = nullptr;
	bool bResult = true;

	if (!(buff = new uint8_t[MEM_SIZE]))
		bResult = false;

	if (bResult) {
		SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, NULL, myDocumentsFolder);
		_tcscat(myDocumentsFolder, _T("\\dump.bin"));
		for (int i = 0; i < MEM_SIZE; ++i)
			buff[i] = ReadByteForDebug(i);

		fp = _tfopen(myDocumentsFolder, _T("wb"));
		if (fp == nullptr) {
			bResult = false;
		}
	}

	if (bResult) {
		int size = (int)fwrite(buff, (int)sizeof(uint8_t), MEM_SIZE, fp);
		fclose(fp);
		if (size != MEM_SIZE) {
			bResult = false;
		}
	}

	if (buff != nullptr) delete[] buff;
	return bResult;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 「高速ロード」を実行
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Address::CjrQuickLoad(const TCHAR* filename)
{
    FILE *fp = nullptr;
	uint8_t* buff = nullptr;
	bool bResult = true;

    if (!(buff = new uint8_t[MEM_SIZE]))
		bResult = false;

    if (bResult && CheckFileFormat(filename) != 1)
		bResult = false;

	if (bResult) {
		fp = _tfopen(filename, _T("rb"));
		if (fp == nullptr)
			bResult = false;
	}

	int size = 0;
	if (bResult) {
		size = (int)fread(buff, (int)sizeof(uint8_t), MEM_SIZE, fp);
		fclose(fp);
		if (size == 0)
			bResult = false;
	}

	if (bResult) {
		CjrFormat cjr;
		cjr.SetCjrData(buff, size);
		uint8_t* bin = cjr.GetBinDataArray();

		if (cjr.isBasic()) { // BASIC
			int size = cjr.GetBinSize();
			for (int i = 0; i < size; ++i)
				WriteByte(0x801 + i, bin[i]);

			uint16_t a = (uint16_t)(0x801 + cjr.GetBinSize());
			memory[0x71] = (uint8_t)(a >> 8);
			memory[0x72] = (uint8_t)a;
		}
		else { // ML
			int offset;
			if (buff[2] == 0)
				offset = 35;
			else
				offset = 2;

			while (buff[offset] != 0xff && offset < size) {
				int blockSize = buff[++offset];
				if (blockSize == 0) blockSize = 256;
				int start = buff[++offset];
				start <<= 8;
				start += buff[++offset];
				for (int i = 0; i < blockSize; ++i)
					WriteByte(start + i, buff[++offset]);
				offset += 4;
			}
		}
	}

	if (buff != nullptr) delete[] buff;
	return bResult;
}
