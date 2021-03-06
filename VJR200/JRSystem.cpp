// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class JRSystem
// JR-200エミュレートに必要なクラスを管理するクラス
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#ifdef _ANDROID
#include <android/log.h>
#else
#include <Shlobj.h>
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "cereal/types/memory.hpp"
#include "cereal/archives/binary.hpp"
#include <fstream>
#include <sys/stat.h>
#include "JRSystem.h"
#include "VJR200.h"
#include "CjrFormat.h"
#include "Jr2Format.h"
#include "SerializeGlobal.h"

JRSystem::JRSystem()
{
}


JRSystem::~JRSystem()
{
    Dispose();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int JRSystem::Init()
{
    int ret = 0;
    stateSave = -1;
    stateLoad = -1;

    if (pAddress == nullptr) {
        pAddress = new Address;
        if (pAddress->Init() == false) {
            Dispose();
            return 1;
        }
    }

    if (pCrtc == nullptr) {
        pCrtc = new Crtc;
        if (pCrtc->Init() == false) {
            Dispose();
            return 2;
        }
    }

    if (pMn1544 == nullptr) {
        pMn1544 = new Mn1544;
        if (pMn1544->Init() == false) {
            g_deviceRunning = false;
            _tcscpy(g_pFontFile, _T(""));
        };
    }

    if (pMn1271 == nullptr) {
        pMn1271 = new Mn1271;
        if (pMn1271->Init() == false) {
            Dispose();
            return 4;
        }
    }

#ifndef _ANDROID
    switch (g_prOutput) {
	case (int)PrinterOutput::TEXT:
		pPrinter = new TextPrinter;
		if (pPrinter->Init() == false) {
			Dispose();
			return 5;
		}
		break;
	case (int)PrinterOutput::RAW:
		pPrinter = new RawPrinter;
		if (pPrinter->Init() == false) {
			Dispose();
			return 6;
		}
		break;
	case (int)PrinterOutput::PNG:
		pPrinter = new ImagePrinter;
		if (pPrinter->Init() == false) {
			Dispose();
			return 7;
		}
	}
#endif

    if (pCpu == nullptr)
        pCpu = new m6800_cpu_device();

    if (_tcslen(g_pRomFile) == 0) {
        g_deviceRunning = false;
    }
    else {
        g_deviceRunning = true;
    }

    if (_tcslen(g_pFontFile) == 0) {
        g_deviceRunning = false;
    }
    else {
        g_deviceRunning = true;
    }

    if (!pAddress->LoadRomFile())
        g_deviceRunning = false;

    g_cpuScale = 100;

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JRSystem内で生成したクラスをdelete
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void JRSystem::Dispose()
{
    if (pCpu != nullptr) {
        delete pCpu;
        pCpu = nullptr;
    }
    if (pMn1271 != nullptr) {
        delete pMn1271;
        pMn1271 = nullptr;
    }
    if (pMn1544 != nullptr) {
        delete pMn1544;
        pMn1544 = nullptr;
    }
    if (pCrtc != nullptr) {
        delete pCrtc;
        pCrtc = nullptr;
    }
    if (pAddress != nullptr) {
        delete pAddress;
        pAddress = nullptr;
    }
#ifndef _ANDROID
    if (pPrinter != nullptr) {
		delete pPrinter;
		pPrinter = nullptr;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CPUリセット
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void JRSystem::Reset()
{
    if (g_pTapeFormat != nullptr) {
        delete g_pTapeFormat;
        g_pTapeFormat = nullptr;
    }

    pCpu->device_start();
    pCpu->device_reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JR-200コード実行
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void JRSystem::StepRun()
{
    step = (int)(CLOCK * g_cpuScale * 0.01f / g_refRate);
    switch (g_debug)
    {
        case -1:// -1 = 通常実行
            pCpu->run(step);
            break;
        case 0:// 0 = 停止

            break;
        case 1:// 1 = ステップ実行
            pCpu->run(1);
            g_debug = 0;
			SetWatchList();
            break;
		case 2: // PAUSEを押したとき
			g_debug = 0;
			SetWatchList();
			break;
    }

    if (stateSave >= 0) {
        Save();
        stateSave = -1;
    }

    if (stateLoad >= 0) {
        Load();
        stateLoad = -1;
    }
}

void JRSystem::SetSaveState(int idx)
{
    if (idx < 0 || idx > 9)
        return;

    stateSave = idx;
}

void JRSystem::SetLoadState(int idx)
{
    if (idx < 0 || idx > 9)
        return;

    stateLoad = idx;
}

void JRSystem::Save()
{
    TCHAR pathFname[MAX_PATH];

    GetStateFilePathName(pathFname, stateSave);
    try {
        std::ofstream os(pathFname, std::ios::binary);
        cereal::BinaryOutputArchive archive(os);

        SerializeGlobal global;
        archive(global);
        archive(*pAddress);
        archive(*pCrtc);
        archive(*pMn1271);
        archive(*pMn1544);
        archive(*pCpu);

        if (g_pTapeFormat != nullptr) {
            switch (CheckFileFormat(g_pTapeFormat->GetFileName())) {
                case 1:
                    archive(*((CjrFormat*)g_pTapeFormat));
                    break;
                case 2:
                    archive(*((Jr2Format*)g_pTapeFormat));
                    break;
            }
        }
    }
    catch (...) {
#ifdef _ANDROID
        __android_log_write(ANDROID_LOG_DEBUG, "JRSYstem.Save","ステートセーブに失敗しました");
#else
        MessageBox(g_hMainWnd, _T("ステートセーブに失敗しました"), _T("エラー"), MB_OK);
#endif
    }

#ifndef _ANDROID
    SetMenuItemForStateSaveLoad();
#endif
}


void JRSystem::Load()
{
    TCHAR pathFname[MAX_PATH];

    GetStateFilePathName(pathFname, stateLoad);
    struct _stat buf;
    int st = _tstat(pathFname, &buf);
    if (st != 0) return;

    try {
        std::ifstream is(pathFname, std::ios::binary);
        cereal::BinaryInputArchive archive(is);

        SerializeGlobal global;
        archive(global);
        archive(*pAddress);
        archive(*pCrtc);
        archive(*pMn1271);
        archive(*pMn1544);
        archive(*pCpu);

        if (g_pTapeFormat != nullptr) {
            switch (CheckFileFormat(g_pTapeFormat->GetFileName())) {
                case 1:
                    archive(*((CjrFormat*)g_pTapeFormat));
                    break;
                case 2:
                    archive(*((Jr2Format*)g_pTapeFormat));
                    break;
            }
        }
    }
    catch (...) {
#ifdef _ANDROID
        __android_log_write(ANDROID_LOG_DEBUG, "JRSYstem.Load","ステートロードに失敗しました");
#else
        MessageBox(g_hMainWnd, _T("ステートロードに失敗しました"), _T("エラー"), MB_OK);
#endif

    }
}
