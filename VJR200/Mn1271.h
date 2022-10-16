// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __MN1271_H__
#define __MN1271_H__
#ifndef _ANDROID
#include <MMSystem.h>
#include <dsound.h>
#else
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#endif
#include <cstdint>
#include <vector>
#include <cereal/cereal.hpp>
#include "VJR200.h"

using namespace std;

#ifdef _ANDROID
#define SAMPLE_NUM 735
#define CH_NUM 3
#endif

enum class Reg3 { KACK = 1, KTEST = 2, KSTROBE = 4, INIT = 8, BUSY = 16, PSEL = 32, KEYSOUND = 64, KSTAT = 128 };
enum class Reg9 { KON = 16, SYSINT = 32, USERINT = 64 };
enum class Reg0e { COUNT = 1, PRESCALE = 24, BORROW = 32, IRQ = 64 };
enum class RegSound8 { DATA = 7, PRESCALE = 24, BORROW = 32, IRQ = 64 };
enum class RegSound16 { DATA = 7, PRESCALE = 8, PULSE = 16, BORROW = 32, IRQ = 64 };
enum class Reg1c { PI0_STAT = 1, PI1_STAT = 2, PI2_STAT = 4, SERIAL_STAT = 64 };
enum class Reg1d { TCA_STAT = 1, TCB_STAT = 2, TCC_STAT = 4, TCD_STAT = 8, TCE_STAT = 16, TCF_STAT = 32 };
enum class Reg1e { PI0_MASK = 1, PI1_MASK = 2, PI2_MASK = 4, SERIAL_MASK = 64 };
enum class Reg1f { TCA_MASK = 1, TCB_MASK = 2, TCC_MASK = 4, TCD_MASK = 8, TCE_MASK = 16, TCF_MASK = 32 };
enum class IrqType { KON = 1, SYSINT = 2, USERINT = 4, SERIAL = 128, TCA = 512, TCB = 1024, TCC = 2048, TCD = 4096, TCE = 8192, TCF = 16384 };
enum class PrinterOutput { TEXT, RAW, PNG };
enum class PrinterMaker { EPSON, PANASONIC };

class Mn1271
{
public:
	Mn1271();
	~Mn1271();
	bool Init(void);
	void TickTimerCounter(int cycles);
	uint8_t Read(uint8_t reg);
	uint8_t ReadForDebug(uint8_t r);
	void Write(uint8_t ret, uint8_t val);
	void AssertIrq(int type);
	vector<uint8_t>* saveData = nullptr;

#ifndef _ANDROID
	void SoundDataCopy(int ch, UINT n);
	void CheckStatus();
	void SetVolume(int vol);
	void SetPan(int ch, int vol);
#else
	SLresult SetVolume();
	SLresult SetPan();
#endif
	void SetSoundData(int ch);
	void SoundOn();
	void SoundOff();
	bool GetRemoteStatus();
	bool GetReadStatus();
	bool GetWriteStatus();

	bool debugTcaEnable = true;
	bool debugTcbEnable = true;
	bool debugTccEnable = true;
	bool debugTcdEnable = true;
	bool debugTceEnable = true;
	bool debugTcfEnable = true;

	template <class Archive>
	void save(Archive & ar, std::uint32_t const version) const;
	template <class Archive>
	void load(Archive & ar, std::uint32_t const version);

protected:
	static const int SAMPLING_FREQUENCY = 44100;
	void SetIrqFlag();
	int GetPrescale(int i);

	void Reg3_write(uint8_t val);
	void Reg5_write(uint8_t val);
	void Reg7_write(uint8_t val);
	void Reg9_write(uint8_t val);
	void Reg0d_write(uint8_t val);
	void Reg0e_write(uint8_t val);
	void Reg10_write(uint8_t val);
	void Reg12_write(uint8_t val);
	void Reg14_write(uint8_t val);
	void Reg16_write(uint8_t val);
	void Reg19_write(uint8_t val);
	void Reg1e_write(uint8_t val);
	void Reg1f_write(uint8_t val);
	void SetIrqMask1(int bit, int value);
	void SetIrqMask2(int bit, int value);

	void Reg7_read();

#ifdef _ANDROID
	SLObjectItf engineObject = NULL;
	SLEngineItf engineEngine = NULL;
	SLObjectItf outputMixObject = NULL;
	SLObjectItf bqPlayerObject[CH_NUM] = { NULL, NULL, NULL };
	SLPlayItf bqPlayerPlay[CH_NUM] = { NULL, NULL, NULL };
	SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue[CH_NUM] = { NULL, NULL, NULL };
	SLVolumeItf bqPlayerVolume[CH_NUM] = { NULL, NULL, NULL };
	short buffer[CH_NUM][SOUNDBUFFER_BLOCK_NUM][SAMPLE_NUM];
	int curBuffer[CH_NUM] = { NULL, NULL, NULL };

	void CopyLocalBuffer(short* buffer, int ch);
	static void bqPlayerCallback0(SLAndroidSimpleBufferQueueItf bq, void *context);
	static void bqPlayerCallback1(SLAndroidSimpleBufferQueueItf bq, void *context);
	static void bqPlayerCallback2(SLAndroidSimpleBufferQueueItf bq, void *context);

#else
	LPDIRECTSOUND8		pDS = nullptr;
	LPDIRECTSOUNDBUFFER	pPrimary = nullptr;
	LPDIRECTSOUNDBUFFER	pSecondary[3];
	LPDIRECTSOUNDNOTIFY8 pNotify[3];

	BOOL CreatePrimaryBuffer(void);
	BOOL CreateSoundBuffer(LPDIRECTSOUNDBUFFER *dsb);
	HANDLE bufWriteEvent[3];
#endif

	int16_t GetWave(int ch);
	double SquareWave(int f, double t);
	void PlaySound(int ch);
	void StopSound(int ch);
	void SetFrequency(int ch, int f);
	void WriteLocalBuffer(int ch);

	uint8_t reg[32] = {};
	uint8_t reg17WriteBuf = 0, reg1aWriteBuf = 0;
	uint8_t reg18ReadBuf = 0, reg1bReadBuf = 0;

	unsigned int tcaSetVal = 0;
	unsigned int tcbSetVal = 0;
	unsigned int tccSetVal = 0;
	unsigned int tcdSetVal = 0;
	unsigned int tceHSetVal = 0;
	unsigned int tceLSetVal = 0;
	unsigned int tcfHSetVal = 0;
	unsigned int tcfLSetVal = 0;

	int tcaCountEnable, tcbCountEnable, tccCountEnable, tcdCountEnable, tceCountEnable, tcfCountEnable;
	int tcaCycleCount, tcbCycleCount, tccCycleCount, tcdCycleCount, tceCycleCount, tcfCycleCount;
	int dsBufferSize;
	bool bPlaying[3] = {};
	int frequency[3] = {};
	unsigned int wavePointer[3] = {};
	uint8_t* localBuf[3] = {};
	unsigned int localBufPointer[3] = {};
	double eTime[3];
	unsigned int blockSize;
	double step = 1 / (double)SAMPLING_FREQUENCY;

	bool bSaving = false;
	bool bWrite = false;
	bool bRead = false;
	bool bRemoteOn = false;
	bool bEnterIrq = false;

	int preCpuScale;
};

template<class Archive>
inline void Mn1271::save(Archive & ar, std::uint32_t const version) const
{
	ar(cereal::binary_data(&reg, sizeof(uint8_t) * 32));
	ar(reg17WriteBuf, reg1aWriteBuf, reg18ReadBuf, reg1bReadBuf);
	ar(debugTcaEnable, debugTcbEnable, debugTccEnable, debugTcdEnable, debugTceEnable, debugTcfEnable);
	ar(tcaSetVal, tcbSetVal, tccSetVal, tcdSetVal, tceHSetVal, tceLSetVal, tcfHSetVal, tcfLSetVal);
	ar(tcaCountEnable, tcbCountEnable, tccCountEnable, tcdCountEnable, tceCountEnable, tcfCountEnable);
	ar(tcaCycleCount, tcbCycleCount, tccCycleCount, tcdCycleCount, tceCycleCount, tcfCycleCount);

	for (int i = 0; i < 3; ++i) {
		ar(bPlaying[i]);
		ar(frequency[i]);
	}
	ar(step);
	ar(bWrite, bRead, bRemoteOn, bEnterIrq, preCpuScale);
}

template<class Archive>
inline void Mn1271::load(Archive & ar, std::uint32_t const version)
{
	for (int i = 0; i < 3; ++i) {
		memset(localBuf[i], 0, blockSize);
		localBufPointer[i] = 0;
		eTime[i] = 0;
	}

	ar(cereal::binary_data(&reg, sizeof(uint8_t) * 32));
	ar(reg17WriteBuf, reg1aWriteBuf, reg18ReadBuf, reg1bReadBuf);
	ar(debugTcaEnable, debugTcbEnable, debugTccEnable, debugTcdEnable, debugTceEnable, debugTcfEnable);
	ar(tcaSetVal, tcbSetVal, tccSetVal, tcdSetVal, tceHSetVal, tceLSetVal, tcfHSetVal, tcfLSetVal);
	ar(tcaCountEnable, tcbCountEnable, tccCountEnable, tcdCountEnable, tceCountEnable, tcfCountEnable);
	ar(tcaCycleCount, tcbCycleCount, tccCycleCount, tcdCycleCount, tceCycleCount, tcfCycleCount);
	for (int i = 0; i < 3; ++i) {
		ar(bPlaying[i]);
		ar(frequency[i]);
	}
	ar(step);
	ar(bWrite, bRead, bRemoteOn, bEnterIrq, preCpuScale);
}

#endif