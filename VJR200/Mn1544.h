// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __MN1544_H__
#define __MN1544_H__
#include <cstdint>

enum class InputType {ANK, KANA, GRAPH};

class Mn1544
{
public:
#ifndef _ANDROID
	static const int JPKEY_AT = 0xc0; // VK_OEM_3
	static const int JPKEY_MINUS = 0xbd; // VK_OEM_MINUS
	static const int JPKEY_YEN = 0xdc; // VK_OEM_5
	static const int JPKEY_HAT = 0xde; // VK_OEM_7
	static const int JPKEY_LEFT_KAKUKAKKO = 0xdb; // VK_OEM_5
	static const int JPKEY_SEMICOLON = 0xbb; // VK_OEM_PLUS
	static const int JPKEY_COLON = 0xba; // VK_OEM_1
	static const int JPKEY_RIGHT_KAKUKAKKO = 0xdd; // VK_OEM_6
	static const int JPKEY_COMMA = 0xbc; // VK_OEM_COMMA
	static const int JPKEY_PERIOD = 0xbe; // VK_OEM_PERIOD
	static const int JPKEY_SLASH = 0xbf; // VK_OEM_2
	static const int JPKEY_UNDERBAR = 0xe2; // VK_OEM_102
#endif

    static const int FONTSIZE = 2048;
    static const int IRQ_WAIT_TIME = 100;

    Mn1544();
    ~Mn1544();
    void SetKeyTest(uint8_t reg);
    bool Init();
    void TickCounter(int cycles);
    void StartQuickType(TCHAR* data);
    void StopQuickType();
    uint8_t KeyIn(int w, int l);
    uint8_t KeyScan();
    void AutoType(const char w[]);
    uint8_t fontData[FONTSIZE + 1];
    bool IsQuickType() { return bQuickType; };
    template <class Archive>
    void serialize(Archive & ar, std::uint32_t const version);

#ifndef _ANDROID
	uint8_t ConvertKeyCode(int w);
#endif

#ifdef _ANDROID
    void KeyUp();
#endif

protected:
#ifndef _ANDROID
#define CHAR_BYTE 3
#else
#define CHAR_BYTE 3
#endif
    static const char* rStr1;
    static const char kanaTbl1[5][2 * CHAR_BYTE];
    static const char* rStr2;
    static const char kanaTbl2[19][5][4 * CHAR_BYTE];
    static const char* rStr3;
    static const char kanaTbl3[18][5][4 * CHAR_BYTE];

    void ScanKeyAndPad();
    void ConvertRKana(int key);
    void ClearRomajiKeys();
    void SendKeycodeLater(int code);

    uint8_t scanBuff[3];
    int pointer = 0;
    bool joystick1 = false;
    bool joystick2 = false;
    InputType mode = InputType::ANK;
    bool bInitialized = false;
    bool bScaned = false;
    bool bScanning = false;
    bool bKtested = false;
    bool bBreakOn = true;
    bool bKeyRepeat = false;

    uint8_t preKtest = 0;
    uint8_t preKack = 0;
    uint8_t prepreKtest = 0;
    uint8_t prepreKack = 0;

    char keys[4] = {};
    char errStr[4];
    int curKeysPos = 0;

    bool bAutoTyping = false;
    double dCounter = 0, interval;
    uint8_t* typeWord = nullptr;
    int wordLen, typeCount;
    int preCpuScale;
    bool bQuickType = false;
    int irqCounter = 0;
    int keycode = 0;

    // Androidでのみ使用
    int lastKeyCode;
};

template<class Archive>
inline void Mn1544::serialize(Archive & ar, std::uint32_t const version)
{
    if (version >= 3) {
        ar(cereal::binary_data(scanBuff, sizeof(uint8_t) * 3));
        ar(pointer, joystick1, joystick2, mode, bInitialized, bScaned, bScanning, bKtested, bBreakOn, bKeyRepeat);
        ar(preKtest, preKack, prepreKtest, prepreKack);
        ar(cereal::binary_data(keys, sizeof(char) * 4));
        ar(cereal::binary_data(errStr, sizeof(char) * 4));
        ar(curKeysPos);
        ar(bAutoTyping, dCounter, interval, wordLen, typeCount, preCpuScale, bQuickType);
        ar(irqCounter, keycode);
        // Androidでのみ使用
        ar(lastKeyCode);
    }
    else {
        ar(cereal::binary_data(scanBuff, sizeof(uint8_t) * 3));
        ar(pointer, joystick1, joystick2, mode, bInitialized, bScaned, bScanning, bKtested, bBreakOn, bKeyRepeat);
        ar(preKtest, preKack, prepreKtest, prepreKack);
        ar(cereal::binary_data(keys, sizeof(char) * 4));
        ar(cereal::binary_data(errStr, sizeof(char) * 4));
        ar(curKeysPos);
        ar(bAutoTyping, dCounter, interval, wordLen, typeCount, preCpuScale, bQuickType);
        // Androidでのみ使用
        ar(lastKeyCode);
    }
}
#endif