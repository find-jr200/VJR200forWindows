// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class Mn1271
// 周辺チップMN1271の機能を実装
//
// 機能:割り込み、サウンド、CMT、プリンタ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#ifdef _ANDROID
#include <pthread.h>
#else
#include <Shlobj.h>
#include <Shlwapi.h>
#include <mmsystem.h>
#endif
#include <iostream>
#include <fstream>
#include <math.h>
#include "VJR200.h"
#include "Mn1271.h"
#include "JRSystem.h"
#include "CjrFormat.h"
#include "AnalyzeWave.h"

#pragma comment(lib,"winmm.lib")
#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "dxguid.lib" )

extern JRSystem sys;

using namespace std;

CEREAL_CLASS_VERSION(Mn1271, CEREAL_VER);

#ifndef _ANDROID

Mn1271::Mn1271()
{
}

Mn1271::~Mn1271()
{
    SoundOff();

	for (int i = 0; i < 3; ++i) {
		if (pSecondary[i]) {
			pSecondary[i]->Release();
			pSecondary[i] = nullptr;
		}
	}

	if (pPrimary) {
		pPrimary->Release();
		pPrimary = nullptr;
	}

	if (pDS) {
		pDS->Release();
		pDS = nullptr;
	}

	for (int i = 0; i < 3; ++i) {
		if (bufWriteEvent[i] != NULL)
			CloseHandle(bufWriteEvent[i]);
	}

    for (int i = 0; i < 3; ++i) {
        if (localBuf[i] != nullptr) {
            delete [] localBuf[i];
            localBuf[i] = nullptr;
        }

    }

    if (saveData != nullptr) {
        delete saveData;
        saveData = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool Mn1271::Init(void)
{
	dsBufferSize = (int)(SAMPLING_FREQUENCY * 2 / g_sBufferWriteInterval * SOUNDBUFFER_BLOCK_NUM);
	blockSize = dsBufferSize / SOUNDBUFFER_BLOCK_NUM;

	for (int i = 0; i < 3; ++i) {
		localBuf[i] = new uint8_t[blockSize];
		eTime[i] = 0;
	}

    HRESULT ret;

	for (int i = 0; i < 3; ++i) {
		bufWriteEvent[i] = CreateEvent(NULL, TRUE, TRUE, NULL);
		if (bufWriteEvent[i] == NULL)
			return false;
	}


	// DirectSound8を作成
	ret = DirectSoundCreate8(NULL, &pDS, NULL);
	if (FAILED(ret)) {
		return false;
	}

	// 強調モード
	//ret = pDS->SetCooperativeLevel(g_hMainWnd, DSSCL_PRIORITY);
	ret = pDS->SetCooperativeLevel(g_hMainWnd, DSSCL_EXCLUSIVE | DSSCL_PRIORITY);
	if (FAILED(ret)) {
		return false;
	}

	if (!CreatePrimaryBuffer())
		return false;

	for (int i = 0; i < 3; ++i)
	{
		if (!CreateSoundBuffer(&pSecondary[i]))
			return false;
		ret = pSecondary[i]->QueryInterface(IID_IDirectSoundNotify8, (LPVOID*)&pNotify[i]);
		if (FAILED(ret))
			return false;
		for (int j = 0; j < SOUNDBUFFER_BLOCK_NUM + 1; ++j) {
			g_hEvent[i][j] = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (g_hEvent[i][j] == NULL)
				return false;
		}
	}

	SetVolume(g_soundVolume);
	SetPan(2, g_stereo1);
	SetPan(0, g_stereo2);
	SetPan(1, g_stereo3);

	DSBPOSITIONNOTIFY pn[3][SOUNDBUFFER_BLOCK_NUM + 1];
	for (int i = 0; i < 3; ++i) {
		pn[i][0].dwOffset = 0;
		pn[i][0].hEventNotify = g_hEvent[i][0];

		pn[i][1].dwOffset = dsBufferSize / SOUNDBUFFER_BLOCK_NUM;
		pn[i][1].hEventNotify = g_hEvent[i][1];

		pn[i][2].dwOffset = dsBufferSize / SOUNDBUFFER_BLOCK_NUM * 2;
		pn[i][2].hEventNotify = g_hEvent[i][2];

		pn[i][3].dwOffset = dsBufferSize / SOUNDBUFFER_BLOCK_NUM * 3;
		pn[i][3].hEventNotify = g_hEvent[i][3];

		pn[i][4].dwOffset = dsBufferSize / SOUNDBUFFER_BLOCK_NUM * 4;
		pn[i][4].hEventNotify = g_hEvent[i][4];

		pn[i][SOUNDBUFFER_BLOCK_NUM].dwOffset = DSBPN_OFFSETSTOP;
		pn[i][SOUNDBUFFER_BLOCK_NUM].hEventNotify = g_hEvent[i][SOUNDBUFFER_BLOCK_NUM];

		ret = pNotify[i]->SetNotificationPositions(SOUNDBUFFER_BLOCK_NUM + 1, pn[i]);
		if (FAILED(ret))
			return false;
	}
    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirectSoundのボリューム設定
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SetVolume(int vol)
{
    float f = log10f(vol * 0.01f) * 2000;
	if (f < -10000) f = -10000;
	if (pPrimary != nullptr) pPrimary->SetVolume((int)f);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirectSoundのパン設定
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SetPan(int ch, int val)
{
    val = (val - 5) * 10;
	float f = 0;
	if (val < 0) {
		f = powf(100.f, log10f((float)-val)) * -1;
	}
	else {
		f = powf(100.f, log10f((float)val));
	}

	if (f < -10000) f = -10000;
	if (f > 10000) f = 10000;
	if (pSecondary[ch] != nullptr) pSecondary[ch]->SetPan((int)f);
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TCA～TCFのカウンタを更新し、割り込みをかける
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::TickTimerCounter(int cycles)
{
    double tickTime = (double)cycles / (CLOCK * g_cpuScale * 0.01);
    int ps = 0;

    // テープロード用カウンタ
    if (((reg[7] & 0x40) != 0) && (g_pTapeFormat != nullptr))
        g_pTapeFormat->TickCounter(cycles);

    // サウンド処理
    for (int i = 0; i < 3; ++i) {
        eTime[i] += tickTime;
    }

    // TCA　シリアル
    ps = reg[0xe] & (uint8_t)(Reg0e::PRESCALE);
    ps = GetPrescale(ps >> 3);
    int tcaCycle = tcaSetVal * ps;

    if (tcaCycle != 0 && tcaCountEnable != 0) {
        tcaCycleCount += cycles;
        if (tcaCycleCount >= tcaCycle && (g_debug == -1 || debugTcaEnable)) {
            AssertIrq((int)(IrqType::TCA));
            tcaCycleCount -= tcaCycle;
            reg[0xe] |= 0x20;
        }
        reg[0xf] = tcaSetVal - (tcaCycleCount / ps);
    }

    // TCB　テンポ
    ps = reg[0x10] & (uint8_t)(Reg0e::PRESCALE);
    ps = GetPrescale(ps >> 3);
    int tcbCycle = tcbSetVal * ps;

    if (tcbCycle != 0 && tcbCountEnable != 0) {
        tcbCycleCount += cycles;
        if (tcbCycleCount >= tcbCycle && (g_debug == -1 || debugTcbEnable)) {
            AssertIrq((int)(IrqType::TCB));
            tcbCycleCount -= tcbCycle;
            reg[0x10] |= 0x20;
        }
        reg[0x11] = tcbSetVal - (tcbCycleCount / ps);
    }

    // TCC　サウンド
	ps = reg[0x12] & (uint8_t)(RegSound8::PRESCALE);
	ps = GetPrescale(ps >> 3);
	int tccCycle = tccSetVal * ps;

	if (tccCycle != 0 && tccCountEnable != 0) {
		tccCycleCount += cycles;
		if (tccCycleCount >= tccCycle && (g_debug == -1 || debugTccEnable)) {
			AssertIrq((int)(IrqType::TCC));
			tccCycleCount -= tccCycle;
			reg[0x12] |= 0x20;
		}
		reg[0x13] = tccSetVal - (tccCycleCount / ps);
	}

    // TCD　サウンド
	ps = reg[0x14] & (uint8_t)(RegSound8::PRESCALE);
	ps = GetPrescale(ps >> 3);
	int tcdCycle = tcdSetVal * ps;

	if (tcdCycle != 0 && tcdCountEnable != 0) {
		tcdCycleCount += cycles;
		if (tcdCycleCount >= tcdCycle && (g_debug == -1 || debugTcdEnable)) {
			AssertIrq((int)(IrqType::TCD));
			tcdCycleCount -= tcdCycle;
			reg[0x14] |= 0x20;
		}
		reg[0x15] = tcdSetVal - (tcdCycleCount / ps);
	}

    // TCE　リアルタイム
    ps = reg[0x16] & (uint8_t)(RegSound16::PRESCALE);
    ps = GetPrescale(ps >> 3);
    int tceCycle = ((tceHSetVal << 8) + tceLSetVal) * ps;

    if (tceCycle != 0 && tceCountEnable == 1) {
        tceCycleCount += cycles;
        if (tceCycleCount >= tceCycle && (g_debug == -1 || debugTceEnable)) {
            AssertIrq((int)(IrqType::TCE));
            tceCycleCount -= tceCycle;
            reg[0x16] |= 0x20;
        }
        int c = ((tceHSetVal << 8) + tceLSetVal) - (tceCycleCount / ps);
        reg[0x17] = c >> 8;
        reg[0x18] = c & 0xff;
    }

    // TCF　サウンド
	ps = reg[0x19] & (uint8_t)(RegSound16::PRESCALE);
	ps = GetPrescale(ps >> 3);
	int tcfCycle = ((tcfHSetVal << 8) + tcfLSetVal) * ps;

	if (tcfCycle != 0 && tcfCountEnable == 1) {
		tcfCycleCount += cycles;
		if (tcfCycleCount >= tcfCycle && (g_debug == -1 || debugTcfEnable)) {
			AssertIrq((int)(IrqType::TCF));
			tcfCycleCount -= tcfCycle;
			reg[0x19] |= 0x20;
		}
		int c = ((tcfHSetVal << 8) + tcfLSetVal) - (tcfCycleCount / ps);
		reg[0x1a] = c >> 8;
		reg[0x1b] = c & 0xff;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IRQ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::AssertIrq(int type)
{
    switch (type)
    {
        case (int)(IrqType::KON) :
            if (reg[0x1e] & (uint8_t)Reg1e::PI0_MASK) {
                reg[0x1c] |= (uint8_t)Reg1c::PI0_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::SYSINT) :
            if (reg[0x1e] & (uint8_t)Reg1e::PI1_MASK) {
                reg[0x1c] |= (uint8_t)Reg1c::PI1_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::USERINT) :
            if (reg[0x1e] & (uint8_t)Reg1e::PI2_MASK) {
                reg[0x1c] |= (uint8_t)Reg1c::PI2_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::SERIAL) :
            if (reg[0x1e] & (uint8_t)Reg1e::SERIAL_MASK) {
                reg[0x1c] |= (uint8_t)Reg1c::SERIAL_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::TCA) :
            if (reg[0x1f] & (uint8_t)Reg1f::TCA_MASK) {
                reg[0x1d] |= (uint8_t)Reg1d::TCA_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::TCB) :
            if (reg[0x1f] & (uint8_t)Reg1f::TCB_MASK) {
                reg[0x1d] |= (uint8_t)Reg1d::TCB_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::TCC) :
            if (reg[0x1f] & (uint8_t)Reg1f::TCC_MASK) {
                reg[0x1d] |= (uint8_t)Reg1d::TCC_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::TCD) :
            if (reg[0x1f] & (uint8_t)Reg1f::TCD_MASK) {
                reg[0x1d] |= (uint8_t)Reg1d::TCD_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::TCE) :
            if (reg[0x1f] & (uint8_t)Reg1f::TCE_MASK) {
                reg[0x1d] |= (uint8_t)Reg1d::TCE_STAT;
                sys.pCpu->irq();
            }
            break;
        case (int)(IrqType::TCF) :
            if (reg[0x1f] & (uint8_t)Reg1f::TCF_MASK) {
                reg[0x1d] |= (uint8_t)Reg1d::TCF_STAT;
                sys.pCpu->irq();
            }
            break;
    }
    SetIrqFlag();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x1C, 0x1DのMSB設定
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SetIrqFlag()
{
    if ((reg[0x1c] & 0x7f) || (reg[0x1d] & 0x7f)) {
        reg[0x1c] |= 0x80;
        reg[0x1d] |= 0x80;
    }
    else {
        reg[0x1c] &= 0x7f;
        reg[0x1d] &= 0x7f;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ読み出し
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Mn1271::Read(uint8_t r)
{
    uint8_t b = 0;

    if (r < 0 || r > 0x1f) return 0;

    switch (r)
    {
        case 0:
            b = reg[r];
            break;
        case 1:
            b = reg[r];
            break;
        case 2:
            b = reg[r];
            break;
        case 3:
            if ((g_prOutput == (int)PrinterOutput::TEXT) ||
                ((g_prOutput == (int)PrinterOutput::RAW) && (g_prMaker == (int)PrinterMaker::PANASONIC)))
                // Panasonic
                b = reg[r] | 0x30;
            else
                // EPSON
                b = reg[r] | 0x10;
            break;
        case 4:
            b = reg[r];
            break;
        case 5:
            b = reg[r];
            break;
        case 6:
            b = reg[r];
            break;
        case 7:
            Reg7_read();
            b = reg[r];
            break;
        case 8:
            b = reg[r];
            break;
        case 9:
            b = reg[r];
            break;
        case 0xa:
            b = reg[r];
            break;
        case 0xb:
            b = reg[r];
            break;
        case 0xc:
            b = reg[r] | 0x20; // SAVE用の細工
            break;
        case 0xd:
            b = reg[r];
            break;
        case 0x0e:
            b = reg[r];
            reg[r] &= ~(uint8_t)(Reg0e::BORROW);
            reg[0x1d] &= 0xfe;
            SetIrqFlag();
            break;
        case 0xf:
            b = reg[r];
            break;
        case 0x10:
            b = reg[r];
            reg[r] &= ~(uint8_t)(Reg0e::BORROW);
            reg[0x1d] &= 0xfd;
            SetIrqFlag();
            break;
        case 0x11:
            b = reg[r];
            break;
        case 0x12:
            b = reg[r];
            reg[r] &= ~(uint8_t)(RegSound8::BORROW);
            reg[0x1d] &= 0xfb;
            SetIrqFlag();
            break;
        case 0x13:
            b = reg[r];
            break;
        case 0x14:
            b = reg[r];
            reg[r] &= ~(uint8_t)(RegSound8::BORROW);
            reg[0x1d] &= 0xf7;
            SetIrqFlag();
            break;
        case 0x15:
            b = reg[r];
            break;
        case 0x16:
            b = reg[r];
            reg[r] &= ~(uint8_t)(RegSound16::BORROW);
            reg[0x1d] &= 0xef;
            SetIrqFlag();
            break;
        case 0x17:
            b = reg[r];
            reg18ReadBuf = reg[0x18];
            break;
        case 0x18:
            b = reg[r];
            //b = reg18ReadBuf;
            break;
        case 0x19:
            b = reg[r];
            reg[r] &= ~(uint8_t)(RegSound16::BORROW);
            reg[0x1d] &= 0xdf;
            SetIrqFlag();
            break;
        case 0x1a:
            b = reg[r];
            reg1bReadBuf = reg[0x1b];
            break;
        case 0x1b:
            b = reg[r];
            //b = reg1bReadBuf;
            break;
        case 0x1c:
            b = reg[r];
            reg[0x1c] = 0;
            SetIrqFlag();
            break;
        case 0x1d:
            b = reg[r];
            break;
        case 0x1e:
            b = reg[r];
            break;
        case 0x1f:
            b = reg[r];
            break;
    }
    return b;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ読み出し（デバッグ用）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t Mn1271::ReadForDebug(uint8_t r)
{
    uint8_t b = 0;

    if (r < 0 || r > 0x1f) return 0;

    switch (r)
    {
        case 0:
            b = reg[r];
            break;
        case 1:
            b = reg[r];
            break;
        case 2:
            b = reg[r];
            break;
        case 3:
            if ((g_prOutput == (int)PrinterOutput::TEXT) ||
                ((g_prOutput == (int)PrinterOutput::RAW) && (g_prMaker == (int)PrinterMaker::PANASONIC)))
                // Panasonic
                b = reg[r] | 0x30;
            else
                // EPSON
                b = reg[r] | 0x10;
            break;
        case 4:
            b = reg[r];
            break;
        case 5:
            b = reg[r];
            break;
        case 6:
            b = reg[r];
            break;
        case 7:
            b = reg[r];
            break;
        case 8:
            b = reg[r];
            break;
        case 9:
            b = reg[r];
            break;
        case 0xa:
            b = reg[r];
            break;
        case 0xb:
            b = reg[r];
            break;
        case 0xc:
            b = reg[r];
            break;
        case 0xd:
            b = reg[r];
            break;
        case 0x0e:
            b = reg[r];
            break;
        case 0xf:
            b = reg[r];
            break;
        case 0x10:
            b = reg[r];
            break;
        case 0x11:
            b = reg[r];
            break;
        case 0x12:
            b = reg[r];
            break;
        case 0x13:
            b = reg[r];
            break;
        case 0x14:
            b = reg[r];
            break;
        case 0x15:
            b = reg[r];
            break;
        case 0x16:
            b = reg[r];
            break;
        case 0x17:
            b = reg[r];
            break;
        case 0x18:
            b = reg[r];
            break;
        case 0x19:
            b = reg[r];
            break;
        case 0x1a:
            b = reg[r];
            break;
        case 0x1b:
            b = reg[r];
            break;
        case 0x1c:
            b = reg[r];
            break;
        case 0x1d:
            b = reg[r];
            break;
        case 0x1e:
            b = reg[r];
            break;
        case 0x1f:
            b = reg[r];
            break;
    }
    return b;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x07読み出し時の個別処理（Load処理用）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg7_read()
{
    uint8_t b;
    bRead = true;
    if ((reg[7] & 0x40) != 0 && g_pTapeFormat != nullptr) {
        b = reg[7] & 0x7f;
        b |= g_pTapeFormat->GetLoadData() * 0x80;
        reg[7] = b;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ書き込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Write(uint8_t r, uint8_t val)
{
    if (r < 0 || r > 0x1f) return;

    switch (r)
    {
        case 0:
            reg[r] = val;
            break;
        case 1:
            reg[r] = val;
            break;
        case 2:
            reg[r] = val;
            break;
        case 3:
            reg[r] = val;
            Reg3_write(val);
            break;
        case 4:
            reg[r] = val;
            break;
        case 5:
            reg[r] = val;
            Reg5_write(val);
            break;
        case 6:
            reg[r] = val;
            break;
        case 7:
            reg[r] = val;
            Reg7_write(val);
            break;
        case 8:
            reg[r] = val & 0x77;
            break;
        case 9:
            reg[r] = val & 0x77;
            Reg9_write(val);
            break;
        case 0xa:
            reg[r] = val & 0x87;
            break;
        case 0xb:
            reg[r] = val & 0x9f;
            break;
        case 0xc:
            reg[r] = val;
            SetIrqMask1(6, (val & 64) ? 1 : 0);
            break;
        case 0xd:
            reg[r] = val;
            Reg0d_write(val);
            break;
        case 0xe:
            reg[r] = val & 0x59;
            reg[0x1d] &= 0xfe;
            SetIrqFlag();

            tcaCycleCount = 0;
            Reg0e_write(val);
            SetIrqMask2(0, (val & 64) ? 1 : 0);
            break;
        case 0xf:
            reg[r] = val;
            tcaSetVal = val;
            if (val == 0)
                tcaSetVal = 0x100;
            tcaCycleCount = 0;
            break;
        case 0x10:
            reg[r] = val & 0x59;
            reg[0x1d] &= 0xfd;
            SetIrqFlag();

            tcbCycleCount = 0;
            Reg10_write(val);
            SetIrqMask2(1, (val & 64) ? 1 : 0);
            break;
        case 0x11:
            reg[r] = val;
            tcbSetVal = val;
            if (val == 0)
                tcbSetVal = 0x100;
            tcbCycleCount = 0;
            break;
        case 0x12:
            reg[r] = val & 0x5f;
            reg[0x1d] &= 0xfb;
            SetIrqFlag();

			tccCycleCount = 0;
            Reg12_write(val);
            SetIrqMask2(2, (val & 64) ? 1 : 0);
            break;
        case 0x13:
            reg[r] = val;
            tccSetVal = val;
            if (val == 0)
                tccSetVal = 0x100;
			tccCycleCount = 0;
            SetSoundData(0);
            break;
        case 0x14:
            reg[r] = val & 0x5f;
            reg[0x1d] &= 0xf7;
            SetIrqFlag();

			tcdCycleCount = 0;
            Reg14_write(val);
            SetIrqMask2(3, (val & 64) ? 1 : 0);
            break;
        case 0x15:
            reg[r] = val;
            tcdSetVal = val;
            if (val == 0)
                tcdSetVal = 0x100;
			tcdCycleCount = 0;
            SetSoundData(1);
            break;
        case 0x16:
            reg[r] = val & 0x5f;
            reg[0x1d] &= 0xef;
            SetIrqFlag();

            tceCycleCount = 0;
            Reg16_write(val);
            SetIrqMask2(4, (val & 64) ? 1 : 0);
            break;
        case 0x17:
            reg17WriteBuf = val;
            break;
        case 0x18:
            reg[0x17] = reg17WriteBuf;
            tceHSetVal = reg17WriteBuf;

            reg[r] = val;
            tceLSetVal = val;

            if (tceHSetVal == 0 && tceLSetVal == 0)
                tceHSetVal = 0x100;
            tceCycleCount = 0;
            break;
        case 0x19:
            reg[r] = val & 0x5f;
            reg[0x1d] &= 0xdf;
            SetIrqFlag();

			tcfCycleCount = 0;
            Reg19_write(val);
            SetIrqMask2(5, (val & 64) ? 1 : 0);
            break;
        case 0x1a:
            reg1aWriteBuf = val;
            break;
        case 0x1b:
            reg[0x1a] = reg1aWriteBuf;
            tcfHSetVal = reg1aWriteBuf;

            reg[r] = val;
            tcfLSetVal = val;

            if (tcfHSetVal == 0 && tcfLSetVal == 0)
                tcfHSetVal = 0x100;
			tcfCycleCount = 0;
            SetSoundData(2);
            break;
        case 0x1c:
            break;
        case 0x1d:
            break;
        case 0x1e:
            reg[r] = val & 0x47;
            Reg1e_write(val);
            break;
        case 0x1f:
            reg[r] = val & 0x3f;
            Reg1f_write(val);
            break;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x1e書きかえ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SetIrqMask1(int bit, int value)
{
    uint8_t tmp = reg[0x1e];
    int mask = ~(1 << bit);
    tmp &= mask;
    reg[0x1e] = tmp | (value << bit);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x1f書きかえ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SetIrqMask2(int bit, int value)
{
    uint8_t tmp = reg[0x1f];
    int mask = ~(1 << bit);
    tmp &= mask;
    reg[0x1f] = tmp | (value << bit);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x03書き込み時の個別処理（MN1544へキーボード処理）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg3_write(uint8_t val)
{
    sys.pMn1544->SetKeyState(val);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x05書き込み時の個別処理（プリンタ出力）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg5_write(uint8_t val)
{
#ifndef _ANDROID
    val = ~val;

	sys.pPrinter->Write(val);

	return;
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x07書き込み時の個別処理（CMT処理）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg7_write(uint8_t val)
{
    if ((val & 0x40) != 0 && bRemoteOn == false) {
        if (g_bOverClockLoad) {
            preCpuScale = g_cpuScale;
            g_cpuScale = CPU_SPEED_MAX;
        }
        bRemoteOn = true;
        if (g_pTapeFormat != nullptr)
            g_pTapeFormat->remoteOn();
    }

    if ((val & 0x40) == 0 && bRemoteOn == true) {
        if (g_bOverClockLoad) {
            g_cpuScale = preCpuScale;
        }
        bRemoteOn = false;
        if (g_pTapeFormat != nullptr)
            g_pTapeFormat->remoteOff();
    }

    // SAVE終了時のファイル書き出し
    if (bSaving && (val & 0x40) == 0) {
        bSaving = false;

        AnalyzeWave a(saveData);
        try {
            a.AnalyzeWavData();
        }
        catch (int){
            delete saveData;
            saveData = nullptr;
            return;
        }
        a.WriteCjrFile();
        delete saveData;
        saveData = nullptr;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x09書き込み時の個別処理（KONの割り込み）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg9_write(uint8_t val)
{
    if (val & (uint8_t)Reg9::KON) {
        AssertIrq((int)(IrqType::KON));
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// レジスタ0x0d書き込み時の個別処理（SAVEデータの書き出し）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg0d_write(uint8_t val)
{
    bWrite = true;

    if (g_pTapeFormat == nullptr || (g_pTapeFormat != nullptr && !_tcsncmp(g_pTapeFormat->GetType(), _T("CJR"), 3))) {
        // CJR書き出し
        if ((reg[0x07] & 0x40) != 0) {
            bSaving = true;
            if (saveData == nullptr)
            {
                saveData = new vector<uint8_t>();
                saveData->push_back(val);
            }
            else {
                saveData->push_back(val);
            }
            reg[0x0c] |= 0x20;
        }
    }
    else {
        // RAW書き出し
        g_pTapeFormat->WriteByte(val);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirectSoundのPLAY実行（エミュレータ実行中は常にPLAY）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SoundOn()
{
#ifndef _ANDROID
    for (int i = 0; i < 3; ++i)
		pSecondary[i]->Play(0, 0, DSBPLAY_LOOPING);
#else
    SLresult result;
    for (int i = 0; i < 3; ++i) {
        if (bqPlayerPlay[i] != NULL)
            result = (*bqPlayerPlay[i])->SetPlayState(bqPlayerPlay[i], SL_PLAYSTATE_PLAYING);
    }
#endif
	g_bSoundOn = true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirectSoundのSTOP実行
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SoundOff()
{
#ifndef _ANDROID
    for (int i = 0; i < 3; ++i)
		pSecondary[i]->Stop();
#else
    SLresult result;
    for (int i = 0; i < 3; ++i) {
        if (bqPlayerPlay[i] != NULL)
            result = (*bqPlayerPlay[i])->SetPlayState(bqPlayerPlay[i], SL_PLAYSTATE_PAUSED);
    }
#endif
    g_bSoundOn = false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// プリスケール値取得
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int Mn1271::GetPrescale(int i)
{
    switch (i)
    {
        case 0:
            return 1;
            break;
        case 1:
            return 8;
            break;
        case 2:
            return 64;
            break;
        case 3:
            return 256;
            break;
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TCA（シリアル設定）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg0e_write(uint8_t val)
{
    if (val == 0x01) reg[0x0c] |= 0x20; // SAVE用
    tcaCountEnable = val & (uint8_t)(Reg0e::COUNT);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TCB（テンポ設定）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg10_write(uint8_t val)
{
    tcbCountEnable = val & (uint8_t)(Reg0e::COUNT);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド設定（引数:0→TCC　1→TCD　2→TCF）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SetSoundData(int ch)
{
    uint8_t val;
    int data, ps, f, c;

    switch (ch) {
        case 0:
            val = reg[0x12];
            data = val & (uint8_t)(RegSound8::DATA);
            ps = val & (uint8_t)(RegSound8::PRESCALE);
            ps = GetPrescale(ps >> 3);
            ps *= 2; // サウンド機能のみ1/2の周波数

            c = tccSetVal;
            f = (int)(CLOCK / ((float)c * ps));
            SetFrequency(0, f);
            break;
        case 1:
            val = reg[0x14];
            data = val & (uint8_t)(RegSound8::DATA);
            ps = val & (uint8_t)(RegSound8::PRESCALE);
            ps = GetPrescale(ps >> 3);
            ps *= 2; // サウンド機能のみ1/2の周波数

            c = tcdSetVal;
            f = (int)(CLOCK / ((float)c * ps));
            SetFrequency(1, f);
            break;
        case 2:
            val = reg[0x19];
            data = val & (uint8_t)(RegSound16::DATA);
            ps = val & (uint8_t)(RegSound16::PRESCALE);
            ps = GetPrescale(ps >> 3);
            ps *= 2; // サウンド機能のみ1/2の周波数

            c = tcfHSetVal << 8;
            c += tcfLSetVal;
            f = (int)(CLOCK / ((float)c * ps));
            SetFrequency(2, f);
            break;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TCC（サウンド設定）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg12_write(uint8_t val)
{
    int data = val & (uint8_t)(RegSound8::DATA);
	tccCountEnable = data ? 1 : 0;
    int ps = val & (uint8_t)(RegSound8::PRESCALE);
    ps = GetPrescale(ps >> 3);
    ps *= 2; // サウンド機能のみ1/2の周波数

    int f = 0;
    unsigned int c = tccSetVal;
    if (data == 6) {
        PlaySound(0);
        if (c != 0) {
            f = (int)(CLOCK / ((float)c * ps));
            if (ps == 128)
                f -= (int)(0.030612244 * f - 2.020408163); // オンチを直す補正式
            SetFrequency(0, f);
        }
        else {
            StopSound(0);
        }
    }
    else {
        StopSound(0);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TCD（サウンド設定）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg14_write(uint8_t val)
{
    int data = val & (uint8_t)(RegSound8::DATA);
	tcdCountEnable = data ? 1 : 0;
	int ps = val & (uint8_t)(RegSound8::PRESCALE);
    ps = GetPrescale(ps >> 3);
    ps *= 2; // サウンド機能のみ1/2の周波数

    int f = 0;
    unsigned int c = tcdSetVal;
    if (data == 6) {
        PlaySound(1);
        if (c != 0) {
            f = (int)(CLOCK / ((float)c * ps));
            if (ps == 128)
                f -= (int)(0.030612244 * f - 2.020408163); // オンチを直す補正式
            SetFrequency(1, f);
        }
        else {
            StopSound(1);
        }
    }
    else {
        StopSound(1);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TCE（リアルタイム設定）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg16_write(uint8_t val)
{
    int data = val & (uint8_t)(RegSound16::DATA);
    if (data == 0) {
        tceCountEnable = 0;
    }
    else {
        tceCountEnable = 1;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TCF（サウンド設定）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg19_write(uint8_t val)
{
    int data = val & (uint8_t)(RegSound16::DATA);
	if (data == 0) {
		tcfCountEnable = 0;
	}
	else {
		tcfCountEnable = 1;
	}

    int ps = val & (uint8_t)(RegSound16::PRESCALE);
    ps = GetPrescale(ps >> 3);
    ps *= 2; // サウンド機能のみ1/2の周波数

    int f = 0;
    unsigned int c = tcfHSetVal << 8;
    c += tcfLSetVal;
    if (data == 6) {
        PlaySound(2);
        if (c != 0) {
            f = (int)(CLOCK / ((float)c * ps));
            SetFrequency(2, f);
        }
        else {
            StopSound(2);
        }
    }
    else {
        StopSound(2);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 割り込みマスク処理1
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg1e_write(uint8_t val)
{
    if (val & 64)
        reg[0xc] |= (uint8_t)Reg0e::IRQ;
    else
        reg[0xc] &= ~(uint8_t)Reg0e::IRQ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 割り込みマスク処理2
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::Reg1f_write(uint8_t val)
{
    if (val & 1)
        reg[0xe] |= (uint8_t)Reg0e::IRQ;
    else
        reg[0xe] &= ~(uint8_t)Reg0e::IRQ;

    if (val & 2)
        reg[0x10] |= (uint8_t)Reg0e::IRQ;
    else
        reg[0x10] &= ~(uint8_t)Reg0e::IRQ;

    if (val & 4)
        reg[0x12] |= (uint8_t)RegSound8::IRQ;
    else
        reg[0x12] &= ~(uint8_t)RegSound8::IRQ;

    if (val & 8)
        reg[0x14] |= (uint8_t)RegSound8::IRQ;
    else
        reg[0x14] &= ~(uint8_t)RegSound8::IRQ;

    if (val & 16)
        reg[0x16] |= (uint8_t)RegSound16::IRQ;
    else
        reg[0x16] &= ~(uint8_t)RegSound16::IRQ;

    if (val & 32)
        reg[0x19] |= (uint8_t)RegSound16::IRQ;
    else
        reg[0x19] &= ~(uint8_t)RegSound16::IRQ;

}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound プライマリバッファ作成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _ANDROID
BOOL Mn1271::CreatePrimaryBuffer(void)
{
	HRESULT ret;
	WAVEFORMATEX wf;

	// プライマリサウンドバッファの作成
	DSBUFFERDESC dsdesc;
	ZeroMemory(&dsdesc, sizeof(DSBUFFERDESC));
	dsdesc.dwSize = sizeof(DSBUFFERDESC);
	dsdesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	dsdesc.dwBufferBytes = 0;
	dsdesc.lpwfxFormat = NULL;
	ret = pDS->CreateSoundBuffer(&dsdesc, &pPrimary, NULL);
	if (FAILED(ret)) {
		return FALSE;
	}

	// プライマリバッファのステータスを決定
	wf.cbSize = sizeof(WAVEFORMATEX);
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 1;
	wf.nSamplesPerSec = SAMPLING_FREQUENCY;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	ret = pPrimary->SetFormat(&wf);
	if (FAILED(ret)) {
		return FALSE;
	}

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound バッファ作成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Mn1271::CreateSoundBuffer(LPDIRECTSOUNDBUFFER *dsb)
{
	HRESULT ret;
	WAVEFORMATEX wf;

	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 1;
	wf.nSamplesPerSec = SAMPLING_FREQUENCY;
	wf.nBlockAlign = 2;
	wf.nAvgBytesPerSec = SAMPLING_FREQUENCY * 2;
	wf.wBitsPerSample = 16;

	DSBUFFERDESC dsdesc;
	ZeroMemory(&dsdesc, sizeof(DSBUFFERDESC));
	dsdesc.dwSize = sizeof(DSBUFFERDESC);
	if (g_bBGPlay)
		dsdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCDEFER | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLPAN | DSBCAPS_GLOBALFOCUS;
	else
		dsdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCDEFER | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLPAN;
	dsdesc.dwBufferBytes = dsBufferSize;
	dsdesc.lpwfxFormat = &wf;
	dsdesc.guid3DAlgorithm = DS3DALG_DEFAULT;
	ret = pDS->CreateSoundBuffer(&dsdesc, dsb, NULL);
	if (FAILED(ret))
		return FALSE;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirectSoundのバッファに再生データコピー ※この関数は別スレッドから実行される
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SoundDataCopy(int ch, UINT n)
{
	HRESULT hr = E_FAIL;
	LPBYTE lpBlockAdd1, lpBlockAdd2;
	DWORD blockSize1, blockSize2;
	BOOL err;

	DWORD d = WaitForSingleObject(bufWriteEvent[ch], INFINITE);

	if (d == WAIT_OBJECT_0) {
		err = ResetEvent(bufWriteEvent[ch]);
		_ASSERT(err);

		if (pSecondary[ch] != nullptr)
			hr = pSecondary[ch]->Lock(blockSize * n, blockSize, (LPVOID*)&lpBlockAdd1, &blockSize1, (LPVOID*)&lpBlockAdd2, &blockSize2, 0);
		if (FAILED(hr))
			return;

		for (unsigned int a = localBufPointer[ch]; a < blockSize; a += 2) {
			int16_t d = GetWave(ch);
			*(localBuf[ch] + a) = (uint8_t)d;
			*(localBuf[ch] + a + 1) = (uint8_t)(d >> 8);
			localBufPointer[ch] += 2;
		}
		_ASSERT(localBufPointer[ch] <= blockSize);

		memcpy_s(lpBlockAdd1, blockSize1, localBuf[ch], blockSize1);

		if (blockSize2 != 0)
			memcpy_s(lpBlockAdd2, blockSize2, localBuf[ch] + blockSize1, blockSize2);

		if (pSecondary[ch] != nullptr)
			pSecondary[ch]->Unlock(lpBlockAdd1, blockSize1, lpBlockAdd2, blockSize2);

		eTime[ch] = 0;
		localBufPointer[ch] = 0;

		err = SetEvent(bufWriteEvent[ch]);
		_ASSERT(err);
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Chごとの再生値返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int16_t Mn1271::GetWave(int ch)
{
    int16_t val;

    double t = 1 / (double)frequency[ch];

    if (bPlaying[ch]) {
        val = (int16_t)SquareWave(frequency[ch], wavePointer[ch] * step) * 7000; // 適当に±7000にしておく
        ++wavePointer[ch];
    }
    else {
        val = 0;
    }

    if (wavePointer[ch] > (unsigned int)(t / step))
        wavePointer[ch] = 0;

    return val;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 矩形波の値を返す（引数:f=周波数　t=時刻）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
double Mn1271::SquareWave(int f, double t)
{
    if (f == 0)
        return 0;

    double tt = 1 / (double)f;
    while (t > tt) t -= tt;

    double span = tt / 2;
    if (t < span)
        return 1.0;
    else
        return -1.0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド再生（Chごと）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::PlaySound(int ch)
{
    WriteLocalBuffer(ch);

    bPlaying[ch] = true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド停止（Chごと）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::StopSound(int ch)
{
    WriteLocalBuffer(ch);

    bPlaying[ch] = false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 生成する波形の周波数設定（Chごと）　スピーカーを保護するため15KHz以下しか再生しない
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::SetFrequency(int ch, int f)
{
    WriteLocalBuffer(ch);

    if (f >= 1 && f <= 15000) {
        frequency[ch] = f;
    }
    else {
        bPlaying[ch] = false;
    }
}

#ifndef _ANDROID
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ローカルバッファ書込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::WriteLocalBuffer(int ch)
{

    DWORD d = WaitForSingleObject(bufWriteEvent[ch], INFINITE);

	if (d == WAIT_OBJECT_0) {
		BOOL err;
		err = ResetEvent(bufWriteEvent[ch]);
		_ASSERT(err);

		if (eTime[ch] >= step) {
			unsigned int c = (int)(eTime[ch] / step);
			for (unsigned int i = 0; i < c; ++i) {
				if (localBufPointer[ch] + 2 > blockSize)
					break;
				eTime[ch] = 0;
				int16_t d = GetWave(ch);
				*(localBuf[ch] + localBufPointer[ch]++) = (uint8_t)d;
				*(localBuf[ch] + localBufPointer[ch]++) = (uint8_t)(d >> 8);
			}
			_ASSERT(localBufPointer[ch] <= blockSize);
		}

		err = SetEvent(bufWriteEvent[ch]);
		_ASSERT(err);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 出力デバイスが切り替わると再生が止まるのでチェックし止まっていたら再生
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Mn1271::CheckStatus()
{
	DWORD status;
	for (int i = 0; i < 3; ++i) {
		if (pSecondary[i] != NULL) {
			pSecondary[i]->GetStatus(&status);
			if ((status & DSBSTATUS_PLAYING) == 0) {
				pSecondary[i]->Play(0, 0, DSBPLAY_LOOPING);
			}
		}
	}
}
#endif


bool Mn1271::GetRemoteStatus()
{
    return bRemoteOn;
}

bool Mn1271::GetReadStatus()
{
    bool bTmp = bRead;
    bRead = false;
    return bTmp;
}

bool Mn1271::GetWriteStatus()
{
    bool bTmp = bWrite;
    bWrite = false;
    return bTmp;
}



#ifdef _ANDROID
#include "Mn1271forAndroid.hxx"
#endif



































