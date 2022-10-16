// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class Mn1544
// キーボード処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <string.h>
#include "JRSystem.h"
#include "Mn1544.h"
#include "VJR200.h"

using namespace std;

extern JRSystem sys;

CEREAL_CLASS_VERSION(Mn1544, CEREAL_VER);

const char* Mn1544::rStr1 = "AIUEO";
const char Mn1544::kanaTbl1[5][2 * CHAR_BYTE] = { "ｱ","ｲ","ｳ","ｴ","ｵ" };

const char* Mn1544::rStr2 = "KSTNHFMYRWGZJDBPVLX";
const char Mn1544::kanaTbl2[19][5][4 * CHAR_BYTE] = { { "ｶ","ｷ","ｸ","ｹ","ｺ" },{ "ｻ","ｼ","ｽ","ｾ","ｿ" },{ "ﾀ","ﾁ","ﾂ","ﾃ","ﾄ" },{ "ﾅ","ﾆ","ﾇ","ﾈ","ﾉ" },
                                                      { "ﾊ","ﾋ","ﾌ","ﾍ","ﾎ" },{ "ﾌｧ","ﾌｨ","ﾌ","ﾌｪ","ﾌｫ" },{ "ﾏ","ﾐ","ﾑ","ﾒ","ﾓ" },{ "ﾔ","","ﾕ","","ﾖ" },{ "ﾗ","ﾘ","ﾙ","ﾚ","ﾛ" },{ "ﾜ","","","","ｦ" },
                                                      { "ｶﾞ","ｷﾞ","ｸﾞ","ｹﾞ","ｺﾞ" },{ "ｻﾞ","ｼﾞ","ｽﾞ","ｾﾞ","ｿﾞ" },{ "ｼﾞｬ","ｼﾞ","ｼﾞｭ","ｼﾞｪ","ｼﾞｮ" },{ "ﾀﾞ","ﾁﾞ","ﾂﾞ","ﾃﾞ","ﾄﾞ" },
                                                      { "ﾊﾞ","ﾋﾞ","ﾌﾞ","ﾍﾞ","ﾎﾞ" },{ "ﾊﾟ","ﾋﾟ","ﾌﾟ","ﾍﾟ","ﾎﾟ" },{ "ｳﾞｧ","ｳﾞｨ","ｳﾞ","ｳﾞｪ","ｳﾞｫ" },{ "ｧ","ｨ","ｩ","ｪ","ｫ" },{ "ｧ","ｨ","ｩ","ｪ","ｫ" } };

const char* Mn1544::rStr3 = "KYSYTYCYNYHYMYRYGYZYDYBYPYXTLTTSSHCH";
const char Mn1544::kanaTbl3[18][5][4 * CHAR_BYTE] = { { "ｷｬ","","ｷｭ","","ｷｮ" },{ "ｼｬ","","ｼｭ","","ｼｮ" },{ "ﾁｬ","","ﾁｭ","","ﾁｮ" },{ "ﾁｬ","","ﾁｭ","","ﾁｮ" },
                                                      { "ﾆｬ","","ﾆｭ","","ﾆｮ" },{ "ﾋｬ","","ﾋｭ","","ﾋｮ" },{ "ﾐｬ","","ﾐｭ","","ﾐｮ" },{ "ﾘｬ","","ﾘｭ","","ﾘｮ" },{ "ｷﾞｬ","","ｷﾞｭ","","ｷﾞｮ" },
                                                      { "ｼﾞｬ","","ｼﾞｭ","","ｼﾞｮ" },{ "ﾁﾞｬ","","ﾁﾞｭ","","ﾁﾞｮ" },{ "ﾋﾞｬ","","ﾋﾞｭ","","ﾋﾞｮ" },{ "ﾋﾟｬ","","ﾋﾟｭ","","ﾋﾟｮ" },
                                                      { "","","ｯ","","" },{ "","","ｯ","","" },{ "","","ﾂ","","" },{ "ｼｬ","ｼ","ｼｭ","ｼｪ","ｼｮ" },{ "ﾁｬ","ﾁ","ﾁｭ","ﾁｪ","ﾁｮ" } };


Mn1544::Mn1544()
{
}


Mn1544::~Mn1544()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Mn1544::Init()
{
    FILE *fp = nullptr;

    pointer = 0;

    fp = _tfopen(g_pFontFile, _T("rb"));
    if (fp == nullptr) {
        _tcscpy(g_pFontFile, _T(""));
        return false;
    }
    fread(fontData, sizeof(fontData[0]), sizeof(fontData), fp);
    fclose(fp);

    fontData[FONTSIZE] = 0; // baud rate

#ifndef _ANDROID
    // グラフキーボード用ビットマップデータ作成
	int c = 0;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 8; ++j) {
			for (int k = 0; k < 14; ++k) {
				g_bits1[c++] = ~fontData[g_gcharCode1[i][k] * 8 + j];
			}
		}
	}
	c = 0;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 8; ++j) {
			for (int k = 0; k < 14; ++k) {
				g_bits2[c++] = ~fontData[g_gcharCode2[i][k] * 8 + j];
			}
		}
	}
#endif
    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// /KSTATの処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::SetKeyState(uint8_t val)
{
    if (!bInitialized) { // 電源投入時
        assert(pointer >= 0 && pointer =< FONTSIZE);
        if (prepreKtest != 0 && preKtest == 0 && (val & 2) != 0 && !bKtested) {
            bKtested = true;
            sys.pAddress->WriteByte(0xc801, fontData[pointer]);
            sys.pAddress->WriteByte(0xc809, 0x10);
            ++pointer;
        }
        else {
            if (prepreKack != 0 && preKack == 0 && ((val & 1) != 0)) {
                sys.pAddress->WriteByte(0xc801, fontData[pointer]);
                sys.pAddress->WriteByte(0xc809, 0x10);
                ++pointer;
            }
        }
        if (pointer == 2049) {
            bInitialized = true;
            bKtested = false;
            pointer = 0;
        }
        prepreKtest = preKtest;
        preKtest = (val & 0x02);
        prepreKack = preKack;
        preKack = (val & 0x01);

    }
    else { // 通常時
        assert(pointer >= 0 && pointer =< 2);
        if (prepreKtest != 0 && preKtest == 0 && (val & 2) != 0 && !bScanning) {
            ScanKeyAndPad();
            bScanning = true;
            sys.pAddress->WriteByte(0xc801, scanBuff[pointer]);
            sys.pAddress->WriteByte(0xc809, 0x10);
            ++pointer;
        }
        else {
            if (prepreKack != 0 && preKack == 0 && ((val & 1) != 0) && bScanning) {
                sys.pAddress->WriteByte(0xc801, scanBuff[pointer]);
                sys.pAddress->WriteByte(0xc809, 0x10);
                ++pointer;
            }
        }
        if (pointer == 3) {
            bScanning = false;
            pointer = 0;
        }
        prepreKtest = preKtest;
        preKtest = (val & 2);
        prepreKack = preKack;
        preKack = (val & 1);
    }
    return;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// キー入力処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Mn1544::KeyIn(int w, int l)
{
    uint8_t c = 0;

    if (bAutoTyping) return 0;

    bKeyRepeat = (l & 0x40000000) ? true : false;
    if ((w == VK_ESCAPE || w == VK_F11) && bBreakOn == true) {
        c = 3;
        mode = InputType::ANK;
        sys.pCpu->nmi();
    } else {
#ifndef _ANDROID
        c = ConvertKeyCode((int)w);
#else
        c = (int)w;
        lastKeyCode = c;
#endif
        if (c != 0) {
            sys.pAddress->WriteByte(0xc801, c);
            sys.pAddress->WriteByte(0xc809, 0x10);
        }
    }

    return c;
}

#ifdef _ANDROID
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// キー入力処理 Android のみ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::KeyUp()
{
    lastKeyCode = 0;
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ローマ字カナ変換
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::ConvertRKana(int key)
{
    keys[curKeysPos] = key;
    if (curKeysPos == 0) { // 1文字で確定
        const char* s = strchr(rStr1, key);
        if (s != nullptr) {
            AutoType((const char*)kanaTbl1[(int)(s - rStr1)]);
            ClearRomajiKeys();
            return;
        }
        else if (key == 'Q'){
            strcpy(errStr, "Q");
            AutoType(errStr);
            ClearRomajiKeys();
            return;
        }
    }
    else if (curKeysPos == 1) { // 2文字で確定
        if (keys[0] == keys[1]) {
            if (keys[0] == 'N') {
                AutoType("ﾝ");
                ClearRomajiKeys();
            }
            else {
                AutoType("ｯ");
            }
            return;
        }
        else {
            const char* s1 = strchr(rStr1, keys[1]);
            if (s1 != nullptr) {
                const char* s2 = strchr(rStr2, keys[0]);
                if (s2 != nullptr) {
                    const char* tblStr = (const char*)kanaTbl2[(int)(s2 - rStr2)][(int)(s1 - rStr1)];
                    if (strlen(tblStr) != 0) {
                        AutoType(tblStr);
                        ClearRomajiKeys();
                        return;
                    }
                    else {
                        strncpy(errStr, keys, 4);
                        AutoType(errStr);
                        ClearRomajiKeys();
                        return;
                    }
                }
                else {
                    strncpy(errStr, keys, 4);
                    AutoType((const char*)errStr);
                    ClearRomajiKeys();
                    return;
                }
            }
            else if (keys[0] == 'N') {
                keys[0] = keys[1];
                keys[1] = '\0';
                AutoType("ﾝ");
                return;
            }
            else {
                char s[3] = {};
                strncpy(s, keys, 2);
                const char* s2 = strstr(rStr3, s);
                if (s2 == nullptr) {
                    strncpy(errStr, keys, 4);
                    AutoType((const char*)errStr);
                    ClearRomajiKeys();
                    return;
                }
            }
        }
    }
    else { // 3文字で確定
        const char* s1 = strchr(rStr1, keys[2]);
        if (s1 != nullptr) {
            char s[3] = {};
            strncpy(s, keys, 2);
            const char* s2 = strstr(rStr3, s);
            if (s2 != nullptr) {
                const char* tblStr = (const char*)kanaTbl3[(int)(s2 - rStr3) / 2][(int)(s1 - rStr1)];
                if (strlen(tblStr) != 0) {
                    AutoType(tblStr);
                }
                else {
                    strncpy(errStr, keys, 4);
                    AutoType((const char*)errStr);
                }
                ClearRomajiKeys();
                return;
            }
            else {
                strncpy(errStr, keys, 4);
                AutoType((const char*)errStr);
                ClearRomajiKeys();
                return;
            }
        }
        else {
            strncpy(errStr, keys, 4);
            AutoType((const char*)errStr);
            ClearRomajiKeys();
            return;
        }
    }

    if (++curKeysPos == 3) {
        ClearRomajiKeys();
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ローマ字入力用バッファクリア
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::ClearRomajiKeys()
{
    curKeysPos = 0;
    memset(keys, 0, 4);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// キースキャン＆ジョイスティック処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::ScanKeyAndPad()
{
    scanBuff[0] = KeyScan();
    uint8_t j1 = 0xff, j2 = 0xff;

#ifdef _ANDROID
    j1 = g_joystick1;
#else
    JOYINFOEX jInfo;
	jInfo.dwSize = sizeof(JOYINFOEX);
	jInfo.dwFlags = JOY_RETURNALL;

	if (joyGetPosEx(JOYSTICKID1, &jInfo) == JOYERR_NOERROR) {
		if (jInfo.dwXpos < 0x4000) j1 &= ~4;
		if (jInfo.dwXpos > 0xC000) j1 &= ~8;
		if (jInfo.dwYpos < 0x4000) j1 &= ~1;
		if (jInfo.dwYpos > 0xC000) j1 &= ~2;

		// 強制ジョイスティックモードの処理
		if (g_bForcedJoystick) {
			if ((jInfo.dwButtons & g_Joypad1pA) != 0) scanBuff[0] = ConvertKeyCode(g_forcedJoypadA);
			if ((jInfo.dwButtons & g_Joypad1pB) != 0) scanBuff[0] = ConvertKeyCode(g_forcedJoypadB);
			if (jInfo.dwXpos < 0x4000) scanBuff[0] = 0x1d;
			if (jInfo.dwXpos > 0xC000) scanBuff[0] = 0x1c;
			if (jInfo.dwYpos < 0x4000) scanBuff[0] = 0x1e;
			if (jInfo.dwYpos > 0xC000) scanBuff[0] = 0x1f;
		}

		if (!g_b2buttons) {
			if (jInfo.dwButtons != 0)  j1 &= ~16;
		}
		else {
			if ((jInfo.dwButtons & g_Joypad1pA) != 0) j1 &= ~16;
			if ((jInfo.dwButtons & g_Joypad1pB) != 0) j1 &= ~32;
		}
	}

	if (joyGetPosEx(JOYSTICKID2, &jInfo) == JOYERR_NOERROR) {
		if (jInfo.dwXpos < 0x4000) j2 &= ~4;
		if (jInfo.dwXpos > 0xC000) j2 &= ~8;
		if (jInfo.dwYpos < 0x4000) j2 &= ~1;
		if (jInfo.dwYpos > 0xC000) j2 &= ~2;
		if (!g_b2buttons) {
			if (jInfo.dwButtons != 0)  j2 &= ~16;
		}
		else {
			if ((jInfo.dwButtons & g_Joypad2pA) != 0)  j2 &= ~16;
			if ((jInfo.dwButtons & g_Joypad2pB) != 0)  j2 &= ~32;
		}
	}

#endif
    if (g_bForcedJoystick)
        scanBuff[1] = 0xff;
    else
        scanBuff[1] = j1;
    scanBuff[2] = j2;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 自動入力処理（タイマー的処理）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::TickCounter(int cycles)
{
    if (bAutoTyping) {
        double tickTime = (double)cycles / (CLOCK * g_cpuScale * 0.01);
        dCounter += tickTime;
        if (dCounter > interval) {
            uint8_t keyin = typeWord[typeCount++];
            if (bQuickType) {
                if (keyin == 0x0a)
                    keyin = typeWord[typeCount++];
            }
            if (typeCount > wordLen) { // 自動入力終了
                if (bQuickType) {
                    delete[] typeWord;
                    typeWord = nullptr;
                    g_cpuScale = preCpuScale;
                }
                bAutoTyping = false;
                bQuickType = false;
                typeCount = 0;
                typeWord = nullptr;
                return;
            }
            sys.pAddress->WriteByte(0xc801, keyin);
            sys.pMn1271->AssertIrq((int)(IrqType::KON));
            dCounter = 0;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// QUICK TYPE スタート
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::StartQuickType(TCHAR* filename)
{
#ifndef _ANDROID
    HANDLE hFile;
	DWORD dwNumberOfReadBytes;

	hFile = CreateFile(filename,
		GENERIC_READ,
		0,
		0,
		OPEN_EXISTING,
		0,
		0
	);
	if (hFile == INVALID_HANDLE_VALUE) {
		MessageBeep(MB_ICONEXCLAMATION);
		return;
	}

	wordLen = GetFileSize(hFile, NULL);
	if (wordLen > 65536) {
		CloseHandle(hFile);
		MessageBeep(MB_ICONEXCLAMATION);
		return;
	}

	preCpuScale = g_cpuScale;
	g_cpuScale = CPU_SPEED_MAX;
	if (typeWord != nullptr)
		delete[] typeWord;
	typeWord = new uint8_t[wordLen];

	if (!ReadFile(hFile,
		typeWord,
		wordLen / sizeof(typeWord[0]),
		&dwNumberOfReadBytes,
		NULL)) {
		CloseHandle(hFile);
		MessageBeep(MB_ICONEXCLAMATION);
		return;
	};
	CloseHandle(hFile);

	interval = g_quickTypeS * 0.001;
	typeCount = 0;
	bAutoTyping = true;
	bQuickType = true;
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// QUICK TYPE ストップ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::StopQuickType()
{
    g_cpuScale = preCpuScale;
    bQuickType = false;
    bAutoTyping = false;
    delete[] typeWord;
    typeWord = nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CTRL+一字入力
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1544::AutoType(const char w[])
{
    typeWord = (uint8_t*)w;
    typeCount = 0;
    wordLen = (int)strlen(w);
    interval = 0.005;
    bQuickType = false;
    bAutoTyping = true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// キースキャン
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Mn1544::KeyScan()
{
#ifndef _ANDROID
    uint8_t code[256];
	uint8_t c = 0;
	int w = 0;
	bool bHit = false;

	if (!GetKeyboardState(code)) MessageBox(g_hMainWnd, g_strTable[(int)Msg::Keyboard_acquisition_error], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);

	for (w = 8; w <= 13; ++w) {
		if (code[w] & 0x80) {
			bHit = true;
			break;
		}
	}
	if (!bHit) {
		for (w = 27; w <= 90; ++w) {
			if (code[w] & 0x80) {
				bHit = true;
				break;
			}
		}
	}
	if (!bHit) {
		for (w = 186; w <= 226; ++w) {
			if (code[w] & 0x80) {
				bHit = true;
				break;
			}
		}
	}

	if (!bHit)  return 0;
	c = ConvertKeyCode(w);

	return c;
#else
    int c = 0;

    if (keyArray.size() != 0) {
        KeyArray::KeySet set;
        keyArray.getKeySet(&set);
        c = set.charCode;
    }
    return (uint8_t)c;
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WindowsのキーコードをJRのキャラに変換
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _ANDROID
uint8_t Mn1544::ConvertKeyCode(int w)
{
	uint8_t c = 0;
	bool bShift = GetKeyState(VK_SHIFT) < 0 ? true : false;
	bool bCtrl = GetKeyState(VK_CONTROL) < 0 ? true : false;

	// Ctrl+Shift+]: はBreakのOn/Off
	if (w == JPKEY_RIGHT_KAKUKAKKO && bShift && bCtrl) {
		bBreakOn = false;
		return 0;
	}
	if (w == JPKEY_COLON && bShift && bCtrl) {
		bBreakOn = true;
		return 0;
	}

	// 英語キーボード用処理
	if (g_keyboard == 1) {
		switch (w) {
			case VK_OEM_PLUS:
				w = JPKEY_HAT;
				break;
			case VK_OEM_102: // Back Slash
				w = JPKEY_YEN;
				break;
			case VK_OEM_4: // Left KakuKakko
				w = JPKEY_AT;
				break;
			case VK_OEM_6: // Right KakuKakko
				w = JPKEY_LEFT_KAKUKAKKO;
				break;

			case VK_OEM_1: // Semi Colon
				w = JPKEY_SEMICOLON;
				break;
			case VK_OEM_7: // Quotation
				w = JPKEY_COLON;
				break;
			case VK_F7:
				w = JPKEY_RIGHT_KAKUKAKKO;
				break;

			case VK_F8:
				w = JPKEY_UNDERBAR;
				break;
		}
	}

	switch (w)
	{
	case VK_PRIOR:
	case VK_CONVERT:
		c = 0x80; // GRAPH
		mode = InputType::GRAPH;
		break;
	case VK_NEXT:
	case VK_NONCONVERT:
		c = 0x14; // 英数
		mode = InputType::ANK;
		break;
	case VK_END:
	case VK_OEM_COPY:
		c = 0xa0; // カナ
		mode = InputType::KANA;
		break;
	case VK_HOME: // HOME CLS
		if (!bShift) c = 0x0b; else c = 0x0c;
		break;
	case VK_INSERT: // INS
		c = 0x13;
		break;
	case VK_DELETE: // DEL
		c = 0x7f;
		break;
	case VK_UP:
		c = 0x1e;
		break;
	case VK_DOWN:
		c = 0x1f;
		break;
	case VK_LEFT:
		c = 0x1d;
		break;
	case VK_RIGHT:
		c = 0x1c;
		break;
	case VK_RETURN: // RETURN
		c = 0xd;
		break;
	case VK_BACK: // RUBOUT
		if (g_bRomajiKana && curKeysPos != 0)
			keys[curKeysPos--] = '\0';
		else
			c = 0x8;
		break;
	case VK_SPACE:
		c = 0x20;
		break;
	case VK_NUMPAD0:
		c = 0x30;
		break;
	case VK_NUMPAD1:
		c = 0x31;
		break;
	case VK_NUMPAD2:
		c = 0x32;
		break;
	case VK_NUMPAD3:
		c = 0x33;
		break;
	case VK_NUMPAD4:
		c = 0x34;
		break;
	case VK_NUMPAD5:
		c = 0x35;
		break;
	case VK_NUMPAD6:
		c = 0x36;
		break;
	case VK_NUMPAD7:
		c = 0x37;
		break;
	case VK_NUMPAD8:
		c = 0x38;
		break;
	case VK_NUMPAD9:
		c = 0x39;
		break;
	case VK_MULTIPLY:
		c = 0x2a;
		break;
	case VK_ADD:
		c = 0x2b;
		break;
	case VK_SUBTRACT:
		c = 0x2d;
		break;
	case VK_DECIMAL:
		c = 0x2e;
		break;
	case VK_DIVIDE:
		c = 0x2f;
		break;
	}


	if (c == 0 && bCtrl == true) {
		if ((sys.pAddress->ReadByte(0xc803) & 0x80) != 0 || bShift == true) {
			// ニュートラルモード
			if (w == 'A') c = 0x01;
			if (w == 'B') c = 0x02;
			if (w == 'C') c = 0x03;
			if (w == 'D') c = 0x04;
			if (w == 'E') c = 0x05;
			if (w == 'F') c = 0x06;
			if (w == 'G') c = 0x07;
			if (w == 'H') c = 0x08;
			if (w == 'I') c = 0x09;
			if (w == 'J') c = 0x0a;
			if (w == 'K') c = 0x0b;
			if (w == 'L') c = 0x0c;
			if (w == 'M') c = 0x0d;
			if (w == 'N') c = 0x0e;
			if (w == 'O') c = 0x0f;
			if (w == 'P') c = 0x10;
			if (w == 'Q') c = 0x11;
			if (w == 'R') c = 0x12;
			if (w == 'S') c = 0x13;
			if (w == 'T') c = 0x14;
			if (w == 'U') c = 0x15;
			if (w == 'V') c = 0x16;
			if (w == 'W') c = 0x17;
			if (w == 'X') c = 0x18;
			if (w == 'Y') c = 0x19;
			if (w == 'Z') c = 0x1a;
			if (w == JPKEY_LEFT_KAKUKAKKO) c = 0x1b;

		}
		else {
			// BASICモード
			if (!bKeyRepeat) {
				if (w == '1') c = 0xc;
				if (w == '2') c = 0xb;
				if (w == 'Z') c = 0x1a;
				if (w == 'X') c = 0x18;
				if (w == 'C') c = 0x03;
				if (w == 'V') c = 0x16;

				if (mode == InputType::ANK) {
					if (w == '3') AutoType("SAVE ");
					if (w == '4') AutoType("LOAD ");
					if (w == '5') AutoType("VERIFY ");
					if (w == '6') AutoType("OPEN ");
					if (w == '7') AutoType("CLOSE ");
					if (w == '8') AutoType("PLAY ");
					if (w == '9') AutoType("TEMPO ");
					if (w == '0') AutoType("BEEP ");
					if (w == JPKEY_MINUS) AutoType("COLOR ");
					if (w == JPKEY_HAT) AutoType("CLEAR ");
					if (w == JPKEY_YEN) AutoType("DELETE ");

					if (w == 'Q') AutoType("GOSUB ");
					if (w == 'W') AutoType("RETURN");
					if (w == 'E') AutoType("END");
					if (w == 'R') AutoType("RUN ");
					if (w == 'T') AutoType("THEN ");
					if (w == 'Y') AutoType("LOCATE ");
					if (w == 'U') AutoType("IF ");
					if (w == 'I') AutoType("INPUT ");
					if (w == 'O') AutoType("PLOT ");
					if (w == 'P') AutoType("PRINT ");
					if (w == JPKEY_AT) AutoType("RANDOMIZE");
					if (w == JPKEY_LEFT_KAKUKAKKO) AutoType("FIND ");

					if (w == 'A') AutoType("AUTO ");
					if (w == 'S') AutoType("STOP");
					if (w == 'D') AutoType("DIM ");
					if (w == 'F') AutoType("FOR ");
					if (w == 'G') AutoType("GOTO ");
					if (w == 'H') AutoType("POKE ");
					if (w == 'J') AutoType("RND(");
					if (w == 'K') AutoType("READ ");
					if (w == 'L') AutoType("LIST ");
					if (w == JPKEY_SEMICOLON) AutoType("CHR$(");
					if (w == JPKEY_COLON) AutoType("REM ");
					if (w == JPKEY_RIGHT_KAKUKAKKO) AutoType("CONT");

					if (w == 'B') AutoType("RESTORE ");
					if (w == 'N') AutoType("NEXT ");
					if (w == 'M') AutoType("MON");
					if (w == JPKEY_COMMA) AutoType("DATA ");
					if (w == JPKEY_PERIOD) AutoType("PEEK(");
					if (w == JPKEY_SLASH) AutoType("HEX$(");
					if (w == JPKEY_UNDERBAR) AutoType("PICK ");

					return c;
				}
			}
			else {
				return 0;
			}
		}
	}


	if (c == 0) {
		switch (mode)
		{
		case InputType::ANK:
			if (w == 0x30 && !bShift) c = 0x30;
			if (w >= 0x31 && w <= 0x39) {
				if (!bShift) c = w;
				else c = w - 0x10;
			}
			if (w >= 0x41 && w <= 0x5A) {
				if (!bShift) c = w + 0x20;
				else c = w;
			}
			if (c == 0) {
				switch (w)
				{
				case JPKEY_COLON: // colon
					if (!bShift) c = 0x3a;	else c = 0x2a;
					break;
				case JPKEY_SEMICOLON: // semi colon
					if (!bShift) c = 0x3b;  else c = 0x2b;
					break;
				case JPKEY_COMMA: // comma
					if (!bShift) c = 0x2c;  else c = 0x3c;
					break;
				case JPKEY_MINUS: // minus
					if (!bShift) c = 0x2d;  else c = 0x3d;
					break;
				case JPKEY_PERIOD: // period
					if (!bShift) c = 0x2e;  else c = 0x3e;
					break;
				case JPKEY_SLASH: // slash
					if (!bShift) c = 0x2f;  else c = 0x3f;
					break;
				case JPKEY_AT: // at
					if (!bShift) c = 0x40;  else c = 0x60;
					break;
				case JPKEY_LEFT_KAKUKAKKO: // kaku kakko
					if (!bShift) c = 0x5b;  else c = 0x7b;
					break;
				case JPKEY_YEN: // yen
					if (!bShift) c = 0x5c;  else c = 0x7c;
					break;
				case JPKEY_RIGHT_KAKUKAKKO: // kaku kakko
					if (!bShift) c = 0x5d;  else c = 0x7d;
					break;
				case JPKEY_HAT: // hat
					if (!bShift) c = 0x5e;  else c = 0x7e;
					break;
				case JPKEY_UNDERBAR: // underbar
					if (bShift) c = 0x5f;
					break;
				}
			}

			break;
		case InputType::KANA:
			if (g_bRomajiKana && w >= 'A' && w <= 'Z') {
				ConvertRKana(w);
				return 0;
			}
			else {
				switch (w)
				{
				case '3':
					if (!bShift) c = (uint8_t)'ｱ'; else c = (uint8_t)'ｧ';
					break;
				case 'E':
					if (!bShift) c = (uint8_t)'ｲ'; else c = (uint8_t)'ｨ';
					break;
				case '4':
					if (!bShift) c = (uint8_t)'ｳ'; else c = (uint8_t)'ｩ';
					break;
				case '5':
					if (!bShift) c = (uint8_t)'ｴ'; else c = (uint8_t)'ｪ';
					break;
				case '6':
					if (!bShift) c = (uint8_t)'ｵ'; else c = (uint8_t)'ｫ';
					break;
				case 'T':
					if (!bShift) c = (uint8_t)'ｶ';
					break;
				case 'G':
					if (!bShift) c = (uint8_t)'ｷ';
					break;
				case 'H':
					if (!bShift) c = (uint8_t)'ｸ';
					break;
				case JPKEY_COLON:
					if (!bShift) c = (uint8_t)'ｹ';
					break;
				case 'B':
					if (!bShift) c = (uint8_t)'ｺ';
					break;
				case 'X':
					if (!bShift) c = (uint8_t)'ｻ';
					break;
				case 'D':
					if (!bShift) c = (uint8_t)'ｼ';
					break;
				case 'R':
					if (!bShift) c = (uint8_t)'ｽ';
					break;
				case 'P':
					if (!bShift) c = (uint8_t)'ｾ';
					break;
				case 'C':
					if (!bShift) c = (uint8_t)'ｿ';
					break;
				case 'Q':
					if (!bShift) c = (uint8_t)'ﾀ';
					break;
				case 'A':
					if (!bShift) c = (uint8_t)'ﾁ';
					break;
				case 'Z':
					if (!bShift) c = (uint8_t)'ﾂ'; else c = (uint8_t)'ｯ';
					break;
				case 'W':
					if (!bShift) c = (uint8_t)'ﾃ';
					break;
				case 'S':
					if (!bShift) c = (uint8_t)'ﾄ';
					break;
				case 'U':
					if (!bShift) c = (uint8_t)'ﾅ';
					break;
				case 'I':
					if (!bShift) c = (uint8_t)'ﾆ';
					break;
				case '1':
					if (!bShift) c = (uint8_t)'ﾇ';
					break;
				case JPKEY_COMMA:
					if (!bShift) c = (uint8_t)'ﾈ'; else c = (uint8_t)'､';
					break;
				case 'K':
					if (!bShift) c = (uint8_t)'ﾉ';
					break;
				case 'F':
					if (!bShift) c = (uint8_t)'ﾊ';
					break;
				case 'V':
					if (!bShift) c = (uint8_t)'ﾋ';
					break;
				case '2':
					if (!bShift) c = (uint8_t)'ﾌ';
					break;
				case JPKEY_HAT:
					if (!bShift) c = (uint8_t)'ﾍ';
					break;
				case JPKEY_MINUS:
					if (!bShift) c = (uint8_t)'ﾎ';
					break;
				case 'J':
					if (!bShift) c = (uint8_t)'ﾏ';
					break;
				case 'N':
					if (!bShift) c = (uint8_t)'ﾐ';
					break;
				case JPKEY_RIGHT_KAKUKAKKO:
					if (!bShift) c = (uint8_t)'ﾑ'; else c = (uint8_t)'｣';
					break;
				case JPKEY_SLASH:
					if (!bShift) c = (uint8_t)'ﾒ'; else c = (uint8_t)'･';
					break;
				case 'M':
					if (!bShift) c = (uint8_t)'ﾓ';
					break;
				case '7':
					if (!bShift) c = (uint8_t)'ﾔ'; else c = (uint8_t)'ｬ';
					break;
				case '8':
					if (!bShift) c = (uint8_t)'ﾕ'; else c = (uint8_t)'ｭ';
					break;
				case '9':
					if (!bShift) c = (uint8_t)'ﾖ'; else c = (uint8_t)'ｮ';
					break;
				case 'O':
					if (!bShift) c = (uint8_t)'ﾗ';
					break;
				case 'L':
					if (!bShift) c = (uint8_t)'ﾘ';
					break;
				case JPKEY_PERIOD:
					if (!bShift) c = (uint8_t)'ﾙ'; else c = (uint8_t)'｡';
					break;
				case JPKEY_SEMICOLON:
					if (!bShift) c = (uint8_t)'ﾚ';
					break;
				case JPKEY_UNDERBAR:
					if (!bShift) c = (uint8_t)'ﾛ';
					break;
				case '0':
					if (!bShift) c = (uint8_t)'ﾜ'; else c = (uint8_t)'ｦ';
					break;
				case 'Y':
					if (!bShift) c = (uint8_t)'ﾝ';
					break;
				case JPKEY_AT:
					if (!bShift) c = (uint8_t)'ﾞ';
					break;
				case JPKEY_LEFT_KAKUKAKKO:
					if (!bShift) c = (uint8_t)'ﾟ'; else c = (uint8_t)'｢';
					break;
				case JPKEY_YEN:
					if (!bShift) c = (uint8_t)'ｰ';
					break;
				}

				if (g_bRomajiKana && ((c >= (uint8_t)'ｦ' && c <= (uint8_t)'ｯ') || (c >= (uint8_t)'ｱ' && c <= (uint8_t)'ﾝ')))
					c = 0;
			}
			break;
		case InputType::GRAPH:
			switch (w)
			{
			case '1':
				if (!bShift) c = 0x81;
				break;
			case '2':
				if (!bShift) c = 0x82;
				break;
			case '3':
				if (!bShift) c = 0x83;
				break;
			case '4':
				if (!bShift) c = 0x84;
				break;
			case '5':
				if (!bShift) c = 0x85;
				break;
			case '6':
				if (!bShift) c = 0x86;
				break;
			case '7':
				if (!bShift) c = 0x87;
				break;
			case '8':
				if (!bShift) c = 0x88;
				break;
			case '9':
				if (!bShift) c = 0x89;
				break;
			case 'M':
				if (!bShift) c = 0x8a; else c = 0x8b;
				break;
			case JPKEY_HAT:
				if (!bShift) c = 0x8c;
				break;
			case 'O':
				if (!bShift) c = 0x8d; else c = 0x9d;
				break;
			case JPKEY_YEN:
				if (!bShift) c = 0x8e;
				break;
			case 'T':
				if (!bShift) c = 0xeb; else c = 0x8f;
				break;
			case 'I':
				if (!bShift) c = 0x90; else c = 0xfe;
				break;
			case 'A':
				if (!bShift) c = 0x91; else c = 0xf1;
				break;
			case 'S':
				if (!bShift) c = 0x92; else c = 0xf7;
				break;
			case 'D':
				if (!bShift) c = 0x93; else c = 0xe5;
				break;
			case 'F':
				if (!bShift) c = 0x94; else c = 0xf2;
				break;
			case 'G':
				if (!bShift) c = 0x95; else c = 0xf4;
				break;
			case 'H':
				if (!bShift) c = 0x96; else	c = 0xf9;
				break;
			case 'J':
				if (!bShift) c = 0x97; else	c = 0xf5;
				break;
			case 'Q':
				if (!bShift) c = 0x98; else c = 0x9e;
				break;
			case 'E':
				if (!bShift) c = 0x99; else c = 0x9f;
				break;
			case 'Y':
				if (!bShift) c = 0x9a;
				break;
			case 'W':
				if (!bShift) c = 0x9b; else c = 0xff;
				break;
			case 'R':
				if (!bShift) c = 0xec; else c = 0x9c;
				break;
			case 'P':
				if (!bShift) c = 0xe0; else c = 0xfc;
				break;
			case 'Z':
				if (!bShift) c = 0xfa; else c = 0xe1;
				break;
			case 'B':
				if (!bShift) c = 0xe2; else c = 0xe7;
				break;
			case 'C':
				if (!bShift) c = 0xe3; else c = 0xe4;
				break;
			case 'V':
				if (!bShift) c = 0xf6; else c = 0xe6;
				break;
			case 'N':
				if (!bShift) c = 0xee; else c = 0xe8;
				break;
			case 'U':
				if (!bShift) c = 0xe9;
				break;
			case JPKEY_AT:
				if (!bShift) c = 0xea;
				break;
			case JPKEY_MINUS:
				if (!bShift) c = 0xed;
				break;
			case 'K':
				if (!bShift) c = 0xef; else c = 0xfb;
				break;
			case 'L':
				if (!bShift) c = 0xf0; else c = 0xfd;
				break;
			case 'X':
				if (!bShift) c = 0xf8;  else c = 0xf3;
				break;
			}
			break;
		}
	}
	return c;
}

#endif