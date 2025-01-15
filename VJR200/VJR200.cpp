// license:BSD-3-Clause
// copyright-holders:FIND
// VJR200.cpp : アプリケーションのエントリ ポイントを定義します。
//
#include "stdafx.h"
#include <WindowsX.h>
#include <Unknwn.h>  
#include <gdiplus.h>
#include <atlbase.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <VersionHelpers.h>
#include <Commdlg.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <Shlobj.h>
#include <shellapi.h>
#include <algorithm>
#include "JRSystem.h"
#include "VJR200.h"
#include "CjrFormat.h"
#include "AppSetting.h"
#include "AppSettingXml.h"
#include "OptionDialog.h"
#include "Jr2Format.h"
#include "DiskImageDlg.h"

#include <sstream>
#include <codecvt>

#pragma comment(lib, "imm32.lib")
#pragma comment (lib, "gdiplus.lib") 
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

// 定数
#define MAX_LOADSTRING 100
const int CLOCK = 1339285;
const int BITMAP_W = 320;
const int BITMAP_H = 224;
const float REAL_WRATIO = 0.85f;
const int STR_RESOURCE_NUM = 44;
const unsigned int RECENT_FILES_NUM = 10;
const int CJR_FILE_MAX = 65536;
const int D88_FILE_MAX = 2000000;
const int CPU_SPEED_MAX = 1000;
const int BREAKPOINT_NUM = 12;
const int RW_BREAKPOINT_NUM = 5;
const TCHAR* APPDATA_PATH = _T("\\FIND_JR\\VJR200\\");
const unsigned int CEREAL_VER = 3;
const unsigned int MINIMUM_READABLE_VERSION = 2;
const int JUMP_HISTORY_SIZE = 24;

// グラフキーボード用テーブル
const uint8_t g_gcharCode1[][14] = { 
{ 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0,    0xed, 0x8c, 0x8e, 0 },
{ 0x98, 0x9b, 0x99, 0xec, 0xeb, 0x9a, 0xe9, 0x90, 0x8d, 0xe0, 0xea, 0,    0,    0 },
{ 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0xef, 0xf0, 0,    0,    0,    0,    0 },
{ 0xfa, 0xf8, 0xe3, 0xf6, 0xe2, 0xee, 0x8a, 0,    0,    0,    0,    0,    0,    0 } };
const uint8_t g_gcharCode2[][14] = { 
{ 0x9e, 0xff, 0x9f, 0x9c, 0x8f, 0   , 0,    0xfe, 0x9d, 0xfc, 0,    0,    0,    0 },
{ 0xf1, 0xf7, 0xe5, 0xf2, 0xf4, 0xf9, 0xf5, 0xfb, 0xfd, 0,    0,    0,    0,    0 },
{ 0xe1, 0xf3, 0xe4, 0xe6, 0xe7, 0xe8, 0x8b, 0,    0,    0,    0,    0,    0,    0 } };

// グローバル変数:
#ifndef _ANDROID
HINSTANCE g_hInst;
HWND g_hMainWnd;
HWND g_hMemWnd;
HMODULE g_hMod; // リソースDLLのモジュールハンドル
ID2D1Factory* g_pD2dFactory = nullptr;
IDWriteFactory* g_pDWriteFactory = nullptr;
IWICImagingFactory *pWICImagingFactory = nullptr;
MemWindow* g_pMemWindow = nullptr;
DisasmWindow* g_pDisasmWindow = nullptr;
HANDLE g_hEvent[3][6];
BYTE g_bits1[448], g_bits2[336]; // グラフキーボード用ビットマップデータ
#endif
int g_dramWait = 0;
bool g_deviceRunning = true;
int g_debug = -1; // -1 = 通常実行　　1 = ステップ実行　　0 = 停止　　2 = PAUSEを押したとき
int g_breakPoint[BREAKPOINT_NUM] = {-1, -1,  -1, -1, -1, -1};
int g_rwBreakPoint[RW_BREAKPOINT_NUM][2] = {0};
bool g_rwBKEnableR[RW_BREAKPOINT_NUM] = { 0 };
bool g_rwBKEnableW[RW_BREAKPOINT_NUM] = { 0 };
bool g_bSoundOn = false;
bool g_bMemWindow = false;
bool g_bDisasmWindow = false;
TCHAR g_strTable[STR_RESOURCE_NUM + 1][256]; // [0]は使わない
bool g_bForcedJoystick = false;
int g_vcyncCounter = 0;
ITapeFormat* g_pTapeFormat;
std::map<int, std::wstring> g_debugLabel;
uint16_t g_prePC = 0;
TCHAR g_JumpHistory[JUMP_HISTORY_SIZE][12] = { 0 };
int g_JumpHistory_index = 0;
TCHAR g_RWBreak[10] = { 0 };
bool g_bFddEnabled = false;
int g_rotate = 0;

JRSystem sys;

// ダイアログ・ini保存項目
int g_cpuScale = 100;
int g_viewScale = 2;
bool g_bSmoothing = false;
bool g_bFpsCpu = false;
int g_soundVolume =40;
bool g_bRamExpand1 = false;
bool g_bRamExpand2 = false;
int g_ramInitPattern = 0;
#ifndef _ANDROID
RECT g_windowPos = {100, 100, 0, 0};
#endif 
int g_refRate = 60;
bool g_bRefRateAuto = true;
bool g_bSquarePixel = true;
bool g_bOverClockLoad = false;
int g_quickTypeS = 30;
int g_language = 0;
int g_keyboard = 0;
int g_stereo1 = 5, g_stereo2 = 5, g_stereo3 = 5;
bool g_bBGPlay = false;
int g_prOutput = 0;
int g_prMaker = 0;
int g_prAutoFeed = 1;
deque<wstring> g_rFilesforCJRSet;
deque<wstring> g_rFilesforQLoad;
deque<wstring> g_macroStrings;
TCHAR g_pRomFile[MAX_PATH];
TCHAR g_pFontFile[MAX_PATH];
TCHAR g_pFdRomFile[MAX_PATH];
bool g_b2buttons = false;
int g_Joypad1pA = 0, g_Joypad1pB = 0, g_Joypad2pA = 0, g_Joypad2pB = 0;
int g_forcedJoypadA = 0x20, g_forcedJoypadB = 0x20;
int g_sBufferWriteInterval = 60;
int g_bCmtAddBlank = 1;
bool g_bRomajiKana = false;
bool g_bPrinterLed = true;
bool g_bStatusBar = true;
bool g_bDetachFdd = false;

static const int STATESAVE_SUBMENU_POS = 15;
static const int STATELOAD_SUBMENU_POS = 16;
static const int RFILES_SUBMENU_POS = 17;
static const int STATUSBAR_HEIGHT = 20;
static const int WATCHVAR_NAME_LEN = 21;
static const int DEBUGLABEL_NAME_LEN = 21;

static const int MACRO_MAX_LEN = 81;

static HIMC hImc;
static HANDLE hFpsTimer = NULL;
static HANDLE hQuickTypeTimer = NULL;
static HANDLE hTimerQueue = NULL;
static HWND hWndDebug = NULL;
static HWND hWndVGraphKeyb = NULL;
static HWND hWndMacro = NULL; 
static HANDLE hSoundThread0 = NULL, hSoundThread1 = NULL, hSoundThread2 = NULL;
static DWORD preTime;
static WPARAM preKey;
static HBITMAP hBitmap1, hBitmap2;
static TCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
static TCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
static bool bFullScreen = false;
static bool bDebugWindow = false;
static bool bVGraphKeyb = false;
static bool bMacroWindow = false;
static int fps = 0;
static TCHAR tmpFileName[MAX_PATH];
static uint8_t ioReg[32];
static int debugDrawCount;
static char macroTypeBuff[MACRO_MAX_LEN];
static WNDPROC DefStaticProc;

static AppSetting setting;
static AppSettingXml settingXml;

struct WatchVar {
	TCHAR* name;
	int size;
	int address;

	WatchVar()
	{
		name = new TCHAR[WATCHVAR_NAME_LEN];
	};

	~WatchVar()
	{
		delete [] name;
	}
};

static std::vector<WatchVar *> watchVarList;
static HHOOK hDebugHook = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// エントリ・ポイント
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (settingXml.Init()) {
		settingXml.Read();
	} 
	else {
		if (setting.Init()) {
			setting.Read();
		}
	}

	LCID id = GetUserDefaultLCID();

	if (g_language == 0) {
		if (id == 1041) {
			g_hMod = LoadLibrary(_T(".\\VJR200_jp.dll"));
		}
		else {
			g_hMod = LoadLibrary(_T(".\\VJR200_en.dll"));
		}
	}
	else if (g_language == 1) {
		g_hMod = LoadLibrary(_T(".\\VJR200_jp.dll"));
	}
	else {
		g_hMod = LoadLibrary(_T(".\\VJR200_en.dll"));
	}

	if (g_hMod == NULL) {
		MessageBox(NULL, _T("Failed to load resource DLL"), g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
		return 0;
	}

	for (int i = 1; i < STR_RESOURCE_NUM + 1; ++i)
		LoadString(g_hMod, i + 2000, g_strTable[i], 256);

	if (!IsWindowsVistaSP2OrGreater()) {
		MessageBox(NULL, g_strTable[(int)Msg::It_will_not_work_on_this_version_of_Windows], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
		return 0;
	}

	if (FAILED(CoInitialize(NULL)))
		return 0;
	hTimerQueue = CreateTimerQueue();
	timeBeginPeriod(1);

	// グローバル文字列を初期化しています。
	LoadString(g_hMod, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(g_hMod, IDC_VJR200, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
		return false;
	}

	// Direct2D, DirectWrite
	HRESULT hr;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2dFactory);
	if (SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(g_pDWriteFactory),
			reinterpret_cast<IUnknown **>(&g_pDWriteFactory)
		);
	}
	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_WICImagingFactory,	NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICImagingFactory));
	}
	if (FAILED(hr))
		return false;

	// アプリケーションの初期化を実行します:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return 0;
	}

	hImc = ImmAssociateContext(g_hMainWnd, 0);
	HACCEL hAccelTable = LoadAccelerators(g_hInst, MAKEINTRESOURCE(IDC_VJR200));
	
	// エミュレータ初期化
	int i;
	if (i = sys.Init()) {
		TCHAR c[30];
		wsprintf(c, g_strTable[(int)Msg::Initialization_failed], i);
		MessageBox(g_hMainWnd, c, g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
		return FALSE;
	}



	// FDD初期化
	if (sys.pFddSystem == nullptr) {
		sys.pFddSystem = new FddSystem();
	}


	if (g_deviceRunning) {
		ValidateRect(g_hMainWnd, NULL);
		SetClassLongPtr(g_hMainWnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(0, 0, 0)));
		sys.Reset();
	}
	else {
		InvalidateRect(g_hMainWnd, NULL, TRUE);
	}

	if (g_bRefRateAuto) {
		int r = GetRefreshRate();
		if (r == 0 || r == 1)
			g_refRate = 60;
		else
			g_refRate = r;
	}

	HMENU hMenu = LoadMenu(g_hMod, MAKEINTRESOURCE(IDC_VJR200));
	SetMenu(g_hMainWnd, hMenu);
	SetMenuItemState(hMenu);

	if (g_bFpsCpu) {
		CreateTimerQueueTimer(&hFpsTimer, hTimerQueue,
			(WAITORTIMERCALLBACK)FpsTimerRoutine, NULL, 0, 1000, 0);
	}

	if (!CreateSoundThreads()) return FALSE;

	MSG msg;
	// メッセージ処理および描画ループ
	while (TRUE) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			else {
				if (!TranslateAccelerator(g_hMainWnd, hAccelTable, &msg)) {
					if ((hWndDebug == NULL || !IsDialogMessage(hWndDebug, &msg)) &&
						(hWndMacro == NULL || !IsDialogMessage(hWndMacro, &msg))) {
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
		}
		else {
			if (g_deviceRunning) {
				WINDOWPLACEMENT wndpl;
				wndpl.length = sizeof(WINDOWPLACEMENT);
				GetWindowPlacement(g_hMainWnd, &wndpl);

				if ((wndpl.showCmd != SW_HIDE) &&
					(wndpl.showCmd != SW_MINIMIZE) &&
					(wndpl.showCmd != SW_SHOWMINIMIZED) &&
					(wndpl.showCmd != SW_SHOWMINNOACTIVE)) {

					if (++debugDrawCount == 7) { // デバッグ情報はVSYNCの1/6回しか更新しない
						RECT rect = { 0, 0, 265, 265 }; // デバッグウィンドウ内レジスタ表示
						if (bDebugWindow) InvalidateRect(hWndDebug, &rect, TRUE);
						rect = { 450, 50, 550, 70 }; // デバッグウィンドウ内breakアドレス表示
						if (bDebugWindow) InvalidateRect(hWndDebug, &rect, TRUE);
						rect = { 450, 100, 550, 120 };// デバッグウィンドウ内R/Wアドレス表示
						if (bDebugWindow) InvalidateRect(hWndDebug, &rect, TRUE);

						if (g_bMemWindow) InvalidateRect(g_pMemWindow->GetHWnd(), NULL, FALSE);
						if (g_bDisasmWindow) InvalidateRect(g_pDisasmWindow->GetHWnd(), NULL, FALSE);
						debugDrawCount = 0;
					}

					// サウンド再生チェック
					if (g_bSoundOn == false) {
						sys.pMn1271->SoundOn();
					}

					DWORD exitCode = 0;
					if (hSoundThread0 != NULL) {
						GetExitCodeThread(hSoundThread0, &exitCode);
						if (exitCode != STILL_ACTIVE) {
							hSoundThread0 = NULL;
							CreateSoundThreads();
						}
					}

					if (hSoundThread1 != NULL) {
						GetExitCodeThread(hSoundThread1, &exitCode);
						if (exitCode != STILL_ACTIVE) {
							hSoundThread1 = NULL;
							CreateSoundThreads();
						}
					}

					if (hSoundThread2 != NULL) {
						GetExitCodeThread(hSoundThread2, &exitCode);
						if (exitCode != STILL_ACTIVE) {
							hSoundThread2 = NULL;
							CreateSoundThreads();
						}
					}

					sys.pMn1271->CheckStatus();

					g_vcyncCounter = 0;

					// 6800命令実行
					sys.StepRun();

					// 描画処理の実行
					++fps;
					sys.pPrinter->CountUp(); // プリンタ・データ受信アイコンを変化させるためのタイマー処理
					sys.pFddSystem->CountUp(); // FDDアイコンを変化させるためのタイマー処理

					if (FAILED(sys.pCrtc->OnRender())) {
						MessageBox(g_hMainWnd, g_strTable[(int)Msg::Creation_of_D2D_device_failed], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
						sys.pCrtc->DiscardDeviceResources();
						sys.Dispose();
						return FALSE;
					}
				}
				else {
					// 最小化時
					if (g_bSoundOn == true)
						sys.pMn1271->SoundOff();

					Sleep(20);
				}
			}
			else {
				Sleep(20);
			}
		}
	}

	// プログラム終了

	if (sys.pFddSystem != nullptr) {
		delete sys.pFddSystem;
		sys.pFddSystem = nullptr;
	}

	sys.pMn1271->SoundOff();
	SafeRelease(&pWICImagingFactory);
	SafeRelease(&g_pDWriteFactory);
	SafeRelease(&g_pD2dFactory);
	Gdiplus::GdiplusShutdown(gdiplusToken);
	DeleteSoundThreads();
	if (g_pTapeFormat != nullptr)
		delete g_pTapeFormat;
	timeEndPeriod(1);
	settingXml.Write();
	DeleteTimerQueueEx(hTimerQueue, NULL);
	sys.Dispose();
	CoUninitialize();
	FreeLibrary(g_hMod);
	return (int)msg.wParam;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
////////////////////////////////////////////////////////////////////////////////////////////////////
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VJR200));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL; // MAKEINTRESOURCE(IDC_VJR200);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウの作成・表示
//
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInst = hInstance; // グローバル変数にインスタンス処理を格納します。
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	RECT rect;

	int statusbarHeight = g_bStatusBar ? STATUSBAR_HEIGHT : 0;

	if (g_bSquarePixel)
		rect = { 0, 0, BITMAP_W * g_viewScale, BITMAP_H * g_viewScale + GetSystemMetrics(SM_CYMENU) + statusbarHeight };
	else
		rect = { 0, 0, (int)(BITMAP_W * g_viewScale * REAL_WRATIO), BITMAP_H * g_viewScale + GetSystemMetrics(SM_CYMENU) + statusbarHeight };

	AdjustWindowRect(&rect, style, FALSE);
	g_windowPos.right = g_windowPos.left + rect.right - rect.left;
	g_windowPos.bottom = g_windowPos.top + rect.bottom - rect.top;
	HWND hWnd = CreateWindow(szWindowClass, szTitle, style,
		g_windowPos.left, g_windowPos.top, g_windowPos.right - g_windowPos.left, g_windowPos.bottom - g_windowPos.top, NULL, NULL, hInstance, nullptr);
	if (!hWnd)
	{
		return FALSE;
	}
	g_hMainWnd = hWnd;
	DragAcceptFiles(hWnd, TRUE);
	ShowWindow(hWnd, SW_RESTORE);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウ・プロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		HMENU hMenu = GetMenu(g_hMainWnd);

		// 選択されたメニューの解析:
		switch (wmId)
		{
		case IDM_HELP_HELP:
			ShellExecute(NULL, _T("open"), g_strTable[(int)Msg::Help_Url], NULL, _T(""), SW_SHOW);
			break;
		case IDM_ABOUT:
			DialogBox(g_hMod, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_FILE_SETCJR:
		{
			TCHAR fileName[MAX_PATH] = {};
			OpenFileDialog(_T("CJR,JR2 file(*.cjr,*.jr2)\0*.cjr;*.jr2\0"), fileName, g_hMainWnd);
			if (_tcslen(fileName) != 0) {
				int type;
				if ((type = CheckFileFormat(fileName)) != 0) {
					AddRecentFiles(fileName, 0);
					MountTapeFile(fileName, type);
					SetMenuItemForStateSaveLoad();
				}
				else {
					MessageBeep(MB_ICONEXCLAMATION);
					MessageBox(NULL, g_strTable[(int)Msg::Invalid_file_format], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
				}
			}
		}
		break;
		case IDM_FILE_QLOAD:
		{
			TCHAR fileName[MAX_PATH] = {};
			OpenFileDialog(_T("CJR file(*.cjr)\0*.cjr\0"), fileName, g_hMainWnd);
			if (_tcslen(fileName) != 0) {
				if (CheckFileFormat(fileName) == 1) {
					AddRecentFiles(fileName, 1);
					sys.pAddress->CjrQuickLoad(fileName);
				}
				else {
					MessageBeep(MB_ICONEXCLAMATION);
					MessageBox(NULL, g_strTable[(int)Msg::Invalid_file_format], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
				}
			}
			break;
		}
		case  IDM_FILE_JR2EJECT:
		{
			if (g_pTapeFormat == nullptr) break;
			delete g_pTapeFormat;
			g_pTapeFormat = nullptr;
			SetMenuItemForStateSaveLoad();
			break;
		}
		case  IDM_FILE_JR2NEW:
		{
			tmpFileName[0] = '\0';
			OPENFILENAME ofn = { 0 };

			ImmAssociateContext(g_hMainWnd, hImc);
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = g_hMainWnd;
			ofn.lpstrFilter = _T("JR2 file(*.jr2)");
			ofn.nMaxCustFilter = 256;
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = tmpFileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

			if (GetSaveFileName(&ofn)) {
				TCHAR chkStr[MAX_PATH];
				_tcscpy(chkStr, tmpFileName);
				_tcsupr(chkStr);
				if (_tcsncmp(_tcsrev(chkStr), _T("2RJ."), 3) != 0)
					_tcscat(tmpFileName, _T(".jr2"));

				if (g_pTapeFormat != nullptr) delete g_pTapeFormat;
				g_pTapeFormat = new Jr2Format();
				if (!g_pTapeFormat->Init(tmpFileName)) {
					delete g_pTapeFormat;
					g_pTapeFormat = nullptr;
					MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
				} else {
					 AddRecentFiles(tmpFileName, 0);
				}
				SetMenuItemForStateSaveLoad();
			}
			hImc = ImmAssociateContext(g_hMainWnd, 0);

			break;
		}
		case IDM_FILE_JR2TOP:
		{
			if (g_pTapeFormat != nullptr)
				g_pTapeFormat->Top();
			break;
		}
		case  IDM_FILE_JR2NEXT:
		{
			if (g_pTapeFormat != nullptr)
				g_pTapeFormat->Next();
			break;
		}
		case  IDM_FILE_JR2PREV:
		{
			if (g_pTapeFormat != nullptr)
				g_pTapeFormat->Prev();
			break;
		}
		case IDM_FILE_PPAGEFEED:
		{
			sys.pPrinter->Finish(true);
			break;
		}
		case IDM_FILE_QUICKTYPE:
		{
			TCHAR filename[MAX_PATH] = {};
			if (OpenFileDialog(_T("Text file(*.txt)\0*.txt\0All(*.*)\0*.*\0\0"), filename, hWnd)) {
				sys.pMn1544->StartQuickType(filename);
			}
			break;
		}
		case IDM_FILE_DEBUGLABEL:
		{
			TCHAR fileName[MAX_PATH] = {};
			OpenFileDialog(_T("Label file(*.*)\0*.*\0"), fileName, g_hMainWnd);
			if (_tcslen(fileName) != 0) {
				if (!SetDebugLabel(fileName))
					MessageBox(NULL, g_strTable[(int)Msg::This_file_cant_be_processed], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
			}
			break;
		}
		case IDM_FILE_RESET:
		{
			if (MessageBox(g_hMainWnd, g_strTable[(int)Msg::Do_you_want_to_reset_it], g_strTable[(int)Msg::Confirmation], MB_YESNO | MB_ICONQUESTION) == IDYES) {
				if (hTimerQueue != NULL)
					DeleteTimerQueueTimer(hTimerQueue, hQuickTypeTimer, NULL);
				sys.pMn1271->SoundOff();
				sys.Dispose();
				g_debug = -1;
				sys.Init();
				sys.Reset();
				SetMenuItemForStateSaveLoad();
			}
			break;
		}
		case IDM_FILE_SAVEMEMIMAGE:
			if (!sys.pAddress->SaveDumpFile())
				MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
			break;

		case IDM_FILE_STATESAVE0:
		case IDM_FILE_STATESAVE1:
		case IDM_FILE_STATESAVE2:
		case IDM_FILE_STATESAVE3:
		case IDM_FILE_STATESAVE4:
		case IDM_FILE_STATESAVE5:
		case IDM_FILE_STATESAVE6:
		case IDM_FILE_STATESAVE7:
		case IDM_FILE_STATESAVE8:
		case IDM_FILE_STATESAVE9:
		{
			int idx = (int)(LOWORD(wParam) - IDM_FILE_STATESAVE0);
			sys.SetSaveState(idx);
			break;
		}
		case IDM_FILE_STATELOAD0:
		case IDM_FILE_STATELOAD1:
		case IDM_FILE_STATELOAD2:
		case IDM_FILE_STATELOAD3:
		case IDM_FILE_STATELOAD4:
		case IDM_FILE_STATELOAD5:
		case IDM_FILE_STATELOAD6:
		case IDM_FILE_STATELOAD7:
		case IDM_FILE_STATELOAD8:
		case IDM_FILE_STATELOAD9:
		{
			int idx = (int)(LOWORD(wParam) - IDM_FILE_STATELOAD0);
			sys.SetLoadState(idx);
			break;
		}
		case IDM_FILE_RFILES_SETCJR0:
		case IDM_FILE_RFILES_SETCJR1:
		case IDM_FILE_RFILES_SETCJR2:
		case IDM_FILE_RFILES_SETCJR3:
		case IDM_FILE_RFILES_SETCJR4:
		case IDM_FILE_RFILES_SETCJR5:
		case IDM_FILE_RFILES_SETCJR6:
		case IDM_FILE_RFILES_SETCJR7:
		case IDM_FILE_RFILES_SETCJR8:
		case IDM_FILE_RFILES_SETCJR9:
		{
			int idx = (int)(LOWORD(wParam) - IDM_FILE_RFILES_SETCJR0);
			if (g_rFilesforCJRSet[idx].size() == 0) break;
			struct _stat buf;
			int st = _tstat(g_rFilesforCJRSet[idx].data(), &buf);
			if (st == 0) {
				int type;
				if ((type = CheckFileFormat(g_rFilesforCJRSet[idx].data())) != 0) {
					MountTapeFile(g_rFilesforCJRSet[idx].data(), type);
					AddRecentFiles(g_rFilesforCJRSet[idx].data(), 0);
				}
				else {
					MessageBeep(MB_ICONEXCLAMATION);
					MessageBox(NULL, g_strTable[(int)Msg::Invalid_file_format], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
					break;
				}
			}
			else {
				if (MessageBox(hWnd, g_strTable[(int)Msg::The_specified_file_can_not_be_opened], g_strTable[(int)Msg::Confirmation], MB_YESNO | MB_ICONQUESTION) == IDYES) {
					g_rFilesforCJRSet.erase(g_rFilesforCJRSet.begin() + idx);
					g_rFilesforCJRSet.push_back(_T(""));
				}
			}
			SetMenuItemForRecentFiles();
			SetMenuItemForStateSaveLoad();
			break;
		}
		case IDM_FILE_RFILES_QLOAD0:
		case IDM_FILE_RFILES_QLOAD1:
		case IDM_FILE_RFILES_QLOAD2:
		case IDM_FILE_RFILES_QLOAD3:
		case IDM_FILE_RFILES_QLOAD4:
		case IDM_FILE_RFILES_QLOAD5:
		case IDM_FILE_RFILES_QLOAD6:
		case IDM_FILE_RFILES_QLOAD7:
		case IDM_FILE_RFILES_QLOAD8:
		case IDM_FILE_RFILES_QLOAD9:
		{
			int idx = (int)(LOWORD(wParam) - IDM_FILE_RFILES_QLOAD0); // 2018.12.18修正
			if (g_rFilesforQLoad[idx].size() == 0) break;

			if (!sys.pAddress->CjrQuickLoad(g_rFilesforQLoad[idx].data())) {
				if (MessageBox(hWnd, g_strTable[(int)Msg::The_specified_file_can_not_be_opened], g_strTable[(int)Msg::Confirmation], MB_YESNO | MB_ICONQUESTION) == IDYES) {
					g_rFilesforQLoad.erase(g_rFilesforQLoad.begin() + idx);
					g_rFilesforQLoad.push_back(_T(""));
				}
			}
			else {
				AddRecentFiles(g_rFilesforQLoad[idx].data(), 1);
			}
			SetMenuItemForRecentFiles();
			break;
		}
		case IDM_FDD_MOUNT_DISKIMAGE:
		{
			DiskImageDlg dlgDiskImage(g_hMainWnd);
			dlgDiskImage.DoModal();

			break;
		}
		case IDM_FDD_DETACHFDD:
		{
			if (g_bDetachFdd) {
				g_bDetachFdd = false;
				CheckMenuItem(hMenu, IDM_FDD_DETACHFDD, MF_UNCHECKED);
			}
			else {
				g_bDetachFdd = true;
				CheckMenuItem(hMenu, IDM_FDD_DETACHFDD, MF_CHECKED);
			}
			break;
		}
		case IDM_VIEW_X1:
		{
			g_viewScale = 1;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		}
		case IDM_VIEW_X2:
		{
			g_viewScale = 2;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		}
		case IDM_VIEW_X3:
		{
			g_viewScale = 3;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		}
		case IDM_VIEW_X4:
		{
			g_viewScale = 4;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		}
		case IDM_VIEW_X5:
		{
			g_viewScale = 5;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		}
		case IDM_VIEW_FULL:
		{
			if (bFullScreen) { // フルスクリーンからウィンドウに戻る
				SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
				HMENU hMenu = LoadMenu(g_hMod, MAKEINTRESOURCE(IDC_VJR200));
				SetMenu(g_hMainWnd, hMenu);
				SetMenuItemState(hMenu);
				SetWindowPos(hWnd, NULL, g_windowPos.left, g_windowPos.top, g_windowPos.right - g_windowPos.left, g_windowPos.bottom - g_windowPos.top, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
				bFullScreen = false;
				sys.pCrtc->Resize(false);
				while (ShowCursor(TRUE) < 0);
			}
			else { // ウィンドウからフルスクリーンに
				GetWindowRect(hWnd, &g_windowPos);
				SetMenu(g_hMainWnd, NULL);
				SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);//ウィンドウのスタイルを変更
				MoveWindow(hWnd, GetSystemMetrics(SM_XVIRTUALSCREEN),
					GetSystemMetrics(SM_YVIRTUALSCREEN),
					GetSystemMetrics(SM_CXVIRTUALSCREEN),
					GetSystemMetrics(SM_CYVIRTUALSCREEN), TRUE);
				bFullScreen = true;
				sys.pCrtc->Resize(true);
				SetTimer(hWnd, 1, 2000, NULL);
			}
			break;
		}
		case IDM_VIEW_SQUAREPIXEL:
			CheckMenuRadioItem(hMenu, IDM_VIEW_SQUAREPIXEL, IDM_VIEW_REAL, IDM_VIEW_SQUAREPIXEL, MF_BYCOMMAND);
			g_bSquarePixel = true;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		case IDM_VIEW_REAL:
			CheckMenuRadioItem(hMenu, IDM_VIEW_SQUAREPIXEL, IDM_VIEW_REAL, IDM_VIEW_REAL, MF_BYCOMMAND);
			g_bSquarePixel = false;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		case IDM_VIEW_SMOOTHING:
			if (g_bSmoothing) {
				g_bSmoothing = false;
				CheckMenuItem(hMenu, IDM_VIEW_SMOOTHING, MF_UNCHECKED);
			}
			else {
				g_bSmoothing = true;
				CheckMenuItem(hMenu, IDM_VIEW_SMOOTHING, MF_CHECKED);
			}
			break;
		case IDM_VIEW_PRINTERLED:
			if (g_bPrinterLed) {
				g_bPrinterLed = false;
				CheckMenuItem(hMenu, IDM_VIEW_PRINTERLED, MF_UNCHECKED);
			}
			else {
				g_bPrinterLed = true;
				CheckMenuItem(hMenu, IDM_VIEW_PRINTERLED, MF_CHECKED);
			}
			DrawMenuBar(hWnd);
			break;
		case IDM_VIEW_R0:
			g_rotate = 0;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		case IDM_VIEW_R90:
			g_rotate = 90;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		case IDM_VIEW_R180:
			g_rotate = 180;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		case IDM_VIEW_R270:
			g_rotate = 270;
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		case IDM_VIEW_STATUSBAR:
			if (g_bStatusBar) {
				g_bStatusBar = false;
				CheckMenuItem(hMenu, IDM_VIEW_STATUSBAR, MF_UNCHECKED);
			}
			else {
				g_bStatusBar = true;
				CheckMenuItem(hMenu, IDM_VIEW_STATUSBAR, MF_CHECKED);
			}
			ChangeWindowSize(hMenu);
			sys.pCrtc->Resize(false);
			break;
		case IDM_TOOL_VGRAPHKEYBOARD:
		{
			if (bVGraphKeyb) {
				DestroyWindow(hWndVGraphKeyb);
				hWndVGraphKeyb = NULL;
				bVGraphKeyb = false;
				CheckMenuItem(hMenu, IDM_TOOL_VGRAPHKEYBOARD, MF_UNCHECKED);
			}
			else {
				hWndVGraphKeyb = CreateDialog(g_hMod, MAKEINTRESOURCE(IDD_VGRAPHKEYBOARD), g_hMainWnd, (DLGPROC)DlgVGraphKeybProc);
				ShowWindow(hWndVGraphKeyb, SW_SHOW);
				bVGraphKeyb = true;
				CheckMenuItem(hMenu, IDM_TOOL_VGRAPHKEYBOARD, MF_CHECKED);
				RECT rect;
				GetWindowRect(hWndVGraphKeyb, &rect);
				MoveWindow(hWndVGraphKeyb, 0, 0, rect.right - rect.left, rect.bottom - rect.top, FALSE);
			}
			break;
		}
		case IDM_TOOL_ROMAJIKANA:
		{
			if (g_bRomajiKana) {
				g_bRomajiKana = false;
				CheckMenuItem(hMenu, IDM_TOOL_ROMAJIKANA, MF_UNCHECKED);
			}
			else {
				g_bRomajiKana = true;
				CheckMenuItem(hMenu, IDM_TOOL_ROMAJIKANA, MF_CHECKED);
			}
			break;
		}

		case IDM_TOOL_MACRO:
		{
			if (bMacroWindow) {
				DestroyWindow(hWndMacro);
				hWndMacro = NULL;
				bMacroWindow = false;
				CheckMenuItem(hMenu, IDM_TOOL_MACRO, MF_UNCHECKED);
			}
			else {
				hWndMacro = CreateDialog(g_hMod, MAKEINTRESOURCE(IDD_MACROWINDOW), g_hMainWnd, (DLGPROC)DlgMacroProc);
				ShowWindow(hWndMacro, SW_SHOW);
				bMacroWindow = true;
				CheckMenuItem(hMenu, IDM_TOOL_MACRO, MF_CHECKED);
			}
			break;
		}

		case IDM_TOOL_FORCEDJOYSTICK:
		{
			if (g_bForcedJoystick) {
				g_bForcedJoystick = false;
				CheckMenuItem(hMenu, IDM_TOOL_FORCEDJOYSTICK, MF_UNCHECKED);
			}
			else {
				g_bForcedJoystick = true;
				CheckMenuItem(hMenu, IDM_TOOL_FORCEDJOYSTICK, MF_CHECKED);
			}
			break;
		}
		case IDM_TOOL_DEBUGWINDOW:
		{
			if (bDebugWindow) {
				DestroyWindow(hWndDebug);
				hWndDebug = NULL;
				bDebugWindow = false;
				CheckMenuItem(hMenu, IDM_TOOL_DEBUGWINDOW, MF_UNCHECKED);
			}
			else {
				hWndDebug = CreateDialog(g_hMod, MAKEINTRESOURCE(IDD_DEBUGWINDOW), g_hMainWnd, (DLGPROC)DlgDebugProc);
				ShowWindow(hWndDebug, SW_SHOW);
				bDebugWindow = true;
				CheckMenuItem(hMenu, IDM_TOOL_DEBUGWINDOW, MF_CHECKED);
				RECT rect;
				GetWindowRect(hWndDebug, &rect);
				MoveWindow(hWndDebug, 0, 0, rect.right - rect.left, rect.bottom - rect.top, FALSE);
			}
			break;
		}
		case IDM_TOOL_MEMORYWINDOW:
		{
			if (g_bMemWindow) {
				g_pMemWindow->Close();
				g_bMemWindow = false;
				CheckMenuItem(hMenu, IDM_TOOL_MEMORYWINDOW, MF_UNCHECKED);
			}
			else {
				g_pMemWindow = new MemWindow();
				g_pMemWindow->Init();
				g_bMemWindow = true;
				CheckMenuItem(hMenu, IDM_TOOL_MEMORYWINDOW, MF_CHECKED);
			}

			break;
		}
		case IDM_TOOL_DISASSEMBLEWINDOW:
		{
			if (g_bDisasmWindow) {
				g_pDisasmWindow->Close();
				g_bDisasmWindow = false;
				CheckMenuItem(hMenu, IDM_TOOL_DISASSEMBLEWINDOW, MF_UNCHECKED);
			}
			else {
				g_pDisasmWindow = new DisasmWindow();
				g_pDisasmWindow->Init();
				g_bDisasmWindow = true;
				CheckMenuItem(hMenu, IDM_TOOL_DISASSEMBLEWINDOW, MF_CHECKED);
			}

			break;
		}
		case IDM_TOOL_ROMFONT:
			DialogBox(g_hMod, MAKEINTRESOURCE(IDD_ROMFONTSETTING), hWnd, (DLGPROC)DlgRomFontProc);
			break;
		case IDM_TOOL_FPSCPU:
		{
			if (g_bFpsCpu)
			{
				DeleteTimerQueueTimer(hTimerQueue, hFpsTimer, NULL);
				SetWindowText(hWnd, _T("VJR-200"));
				CheckMenuItem(hMenu, IDM_TOOL_FPSCPU, MF_UNCHECKED);
				g_bFpsCpu = false;
			}
			else {
				CreateTimerQueueTimer(&hFpsTimer, hTimerQueue,
					(WAITORTIMERCALLBACK)FpsTimerRoutine, NULL, 0, 1000, 0);
				CheckMenuItem(hMenu, IDM_TOOL_FPSCPU, MF_CHECKED);
				g_bFpsCpu = true;
			}

		}
			break;
		case IDM_TOOL_OPTION:
		{
			DialogBox(g_hMod, MAKEINTRESOURCE(IDD_OPTIONDIALOG), hWnd, (DLGPROC)DlgOptionProc);
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	case WM_DROPFILES:
	{
		HDROP hdrop = (HDROP)wParam;
		TCHAR path[MAX_PATH];

		int num = DragQueryFile(hdrop, -1, NULL, 0);
		if (num > 10) {
			DragFinish(hdrop);
			MessageBeep(MB_ICONEXCLAMATION);
			break;
		}

		for (int i = 0; i < num; ++i) {
			DragQueryFile(hdrop, i, path, sizeof(path) / sizeof(TCHAR));
			if (_tcslen(path) != 0) {
				if (CheckFileFormat(path) == 1) {
					AddRecentFiles(path, 1);
					sys.pAddress->CjrQuickLoad(path);
				}
				else {
					MessageBeep(MB_ICONEXCLAMATION);
					MessageBox(g_hMainWnd, g_strTable[(int)Msg::Invalid_file_format], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
					break;
				}
			}
		}
		DragFinish(hdrop);
		break;
	}
	case WM_TIMER:
		switch (wParam)
		{
		case 1:
			// フルスクリーン時のカーソル
			KillTimer(hWnd, 1);
			if (bFullScreen)
				while (ShowCursor(FALSE) >= 0);
			break;
		}
		break;
	case WM_MOUSEMOVE:
		if (bFullScreen) {
			while (ShowCursor(TRUE) < 0);
			SetTimer(hWnd, 1, 2000, NULL);
		}
		break;
	case WM_KEYDOWN:
	{
		// クイックタイプを中止する際のescキーだけを別処理
		if (sys.pMn1544->IsQuickType() && (wParam == VK_ESCAPE || wParam == VK_F11)) {
			sys.pMn1544->StopQuickType();
		}

		// キーリピートを抑制するため、50ms以上たたないと次のリピートを処理しない
		DWORD currentTime = timeGetTime();
		if (currentTime - preTime < 50 && preKey == wParam) break;
		preTime = currentTime;
		preKey = wParam;
		sys.pMn1544->KeyIn((int)wParam, (int)lParam);
	}
		break;
	case WM_LBUTTONDBLCLK:
	{
		SendMessage(hWnd, WM_COMMAND, IDM_VIEW_FULL, 0);
		break;
	}
	case WM_PAINT:
	{
		// ROM,FONTファイルがない場合のエラーメッセージのみここで表示
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		if (_tcslen(g_pRomFile) == 0) {
			RECT rect = { 5, 5, 600, 50 };
			DrawText(hdc, g_strTable[(int)Msg::ROM_file_is_missing_or_incorrect], -1, &rect, 0);
		}
		if (_tcslen(g_pFontFile) == 0) {
			RECT rect = { 5, 50, 600, 100 };
			DrawText(hdc, g_strTable[(int)Msg::FONT_file_is_missing_or_incorrect], -1, &rect, 0);
		}
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_NCPAINT:
	{
		LRESULT lr;

		lr = DefWindowProc(hWnd, message, wParam, lParam);
		if (sys.pPrinter != nullptr && g_bPrinterLed)
			sys.pPrinter->DrawPrinterIcon(false);
		return lr;
	}
	break;
	case WM_NCACTIVATE:
	{
		LRESULT lr;

		lr = DefWindowProc(hWnd, message, wParam, lParam);
		if (wParam && sys.pPrinter != nullptr && g_bPrinterLed)
			sys.pPrinter->DrawPrinterIcon(false);
		return lr;
	}
	break;
	case WM_DESTROY:
		sys.pMn1271->SoundOff();
		GetWindowRect(hWnd, &g_windowPos);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// バージョン情報ボックスのメッセージ ハンドラーです。
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 仮想キーボードのウィンドウ・プロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK DlgVGraphKeybProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_INITDIALOG:
	{
		hBitmap1 = CreateBitmap(112, 32, 1, 1, g_bits1);
		hBitmap2 = CreateBitmap(112, 24, 1, 1, g_bits2);
		return (INT_PTR)TRUE;
		break;
	}
	case WM_CLOSE:
	{
		DeleteObject(hBitmap1);
		DeleteObject(hBitmap2);
		DestroyWindow(hWndVGraphKeyb);
		hWndVGraphKeyb = NULL;
		bVGraphKeyb = false;
		CheckMenuItem(GetMenu(g_hMainWnd), IDM_TOOL_VGRAPHKEYBOARD, MF_UNCHECKED);
		return (INT_PTR)TRUE;
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	{
		int x = GET_X_LPARAM(lp) - 10;
		int y = GET_Y_LPARAM(lp);
		if (x < 0)
			return (INT_PTR)TRUE;
		if (y < 120) {
			y -= 10;
			if (y < 0)
				return (INT_PTR)TRUE;
			x /= 24;
			y /= 24;
			if (x >= 13 || y >= 4)
				return (INT_PTR)TRUE;
			uint8_t c = g_gcharCode1[y][x];
			if (c == 0)
				return (INT_PTR)TRUE;
			sys.pMn1271->IOWrite(1, c);
			sys.pMn1271->AssertIrq((int)(IrqType::KON));
		}
		else {
			y -= 120;
			if (y < 0)
				return (INT_PTR)TRUE;
			x /= 24;
			y /= 24;
			if (x >= 13 || y >= 3)
				return (INT_PTR)TRUE;
			uint8_t c = g_gcharCode2[y][x];
			if (c == 0)
				return (INT_PTR)TRUE;
			sys.pMn1271->IOWrite(1, c);
			sys.pMn1271->AssertIrq((int)(IrqType::KON));
		}
		return (INT_PTR)TRUE;
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		HDC hdcMem1 = CreateCompatibleDC(hdc);
		HDC hdcMem2 = CreateCompatibleDC(hdc);
		SelectObject(hdcMem1, hBitmap1);
		SelectObject(hdcMem2, hBitmap2);
		HPEN hOldPen, hPen = (HPEN)GetStockObject(BLACK_PEN);

		StretchBlt(hdc, 0 + 10 , 0 + 10, 312, 96, hdcMem1, 0, 0, 104, 32, SRCCOPY);
		StretchBlt(hdc, 0 + 10, 0 + 120, 312, 72, hdcMem2, 0, 0, 104, 24, SRCCOPY);
		hOldPen = (HPEN)SelectObject(hdc, hPen);
		for (int i = 0; i <= 13; ++i) {
			MoveToEx(hdc, i * 24 + 10, 0 + 10, NULL);
			LineTo(hdc, i * 24 + 10, 96 + 10);
		}
		for (int i = 0; i <= 4; ++i) {
			MoveToEx(hdc, 0 + 10, i * 24 + 10, NULL);
			LineTo(hdc, 312 + 10, i * 24 + 10);
		}

		for (int i = 0; i <= 13; ++i) {
			MoveToEx(hdc, i * 24 + 10, 0 + 120, NULL);
			LineTo(hdc, i * 24 + 10, 72 + 120);
		}
		for (int i = 0; i <= 3; ++i) {
			MoveToEx(hdc, 0 + 10, i * 24 + 120, NULL);
			LineTo(hdc, 312 + 10, i * 24 + 120);
		}
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
		DeleteDC(hdcMem1);
		DeleteDC(hdcMem2);
		EndPaint(hwnd, &ps);
		return (INT_PTR)TRUE;
		break;
	}
	}
	return (INT_PTR)FALSE;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// マクロウィンドウ　wstring → string変換
//
////////////////////////////////////////////////////////////////////////////////////////////////////
string WStringToString(wstring wstr)
{
	int buffSize = WideCharToMultiByte(CP_OEMCP, 0, wstr.c_str(), -1, (char *)NULL, 0, NULL, NULL);
	char* c = new char[buffSize];
	WideCharToMultiByte(CP_OEMCP, 0, wstr.c_str(), -1, c, buffSize, NULL, NULL);
	string str(c, c + buffSize - 1);
	delete[] c;

	return str;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// string → wstring変換
//
////////////////////////////////////////////////////////////////////////////////////////////////////
std::wstring StringToWString(std::string str)
{
	int buffSize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, (wchar_t*)NULL, 0);
	wchar_t* wc = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wc, buffSize);
	std::wstring oRet(wc, wc + buffSize - 1);
	delete[] wc;
	return oRet;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// マクロウィンドウ　エスケープシーケンス処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void EscapeSequence(char* dst, const char* src)
{
	int srcIdx = 0, dstIdx = 0;
	int srcLen = (int)strlen(src);

	while (srcIdx <= srcLen) {
		if (src[srcIdx] != '\\') {
			dst[dstIdx] = src[srcIdx];
			++srcIdx;
			++dstIdx;
		}
		else {
			++srcIdx;
			if (src[srcIdx] == 'r' || src[srcIdx] == 'R') {
				dst[dstIdx] = 0xd;
				++dstIdx;
				++srcIdx;
			} 
			else if (src[srcIdx] == '\\') {
				dst[dstIdx] = '\\';
				++dstIdx;
				++srcIdx;
			}
			else {
				dst[dstIdx] = '\\';
				++dstIdx;
				dst[dstIdx] = src[srcIdx];
				++dstIdx;
				++srcIdx;
			}
		}
	}
	dst[srcLen] = '\0';
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// マクロウィンドウで使う自動入力
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void AutoTypeMacro(int index) {
	string s = WStringToString(g_macroStrings[index]);
	const char* c = s.c_str();
	EscapeSequence(macroTypeBuff, c);
	sys.pMn1544->AutoType(macroTypeBuff);
	PostMessage(hWndMacro, WM_CLOSE, 0, 0);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// マクロウィンドウのリストボックス　サブクラス化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK ListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_KEYDOWN:
		switch (wp) {
		case VK_DELETE:
		{
			HWND hList = GetDlgItem(hWndMacro, IDC_MACRO_LIST);
			int idx = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
			SendMessage(hList, LB_DELETESTRING, idx, 0);
			g_macroStrings.erase(g_macroStrings.begin() + idx);
			return TRUE;
			break;
		}
		case 48: // '0'
		case 49: // '1'
		case 50:
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57: // '9'
		{
			int code = (int)wp;
			if (code == 48) code = 58;
			int size = (int)g_macroStrings.size();
			int index = code - 49;
			if (index + 1 > size)
				return TRUE;
			AutoTypeMacro(index);
			return TRUE;
			break;
		}
		}
	}
	return CallWindowProc(DefStaticProc, hwnd, msg, wp, lp);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// マクロウィンドウのウィンドウ・プロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK DlgMacroProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	HWND hWndInput = GetDlgItem(hwnd, IDC_MACRO_INPUT);
	HWND hWndList = GetDlgItem(hwnd, IDC_MACRO_LIST);
	static HFONT hNewFont;

	switch (msg) {
	case WM_INITDIALOG:
	{
		int lines = (int)g_macroStrings.size();
		TCHAR strbuff[MACRO_MAX_LEN + 3] = {};

		SendMessage(hWndList, LB_SETHORIZONTALEXTENT, 700, 0);

		for (int i = 0; i < lines; ++i) {
			if (g_macroStrings[i].compare(L"") != 0) {
				wsprintf(strbuff, L"%d:", i + 1);
				_tcscat(strbuff, g_macroStrings[i].c_str());
				SendMessage(hWndList, LB_ADDSTRING, 0, (LPARAM)strbuff);
			}
		}
		// サブクラス化
		DefStaticProc = (WNDPROC)GetWindowLongPtr(hWndList, GWLP_WNDPROC);
		SetWindowLongPtr(hWndList, GWLP_WNDPROC, (LONG_PTR)ListWndProc);

		hNewFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
			_T("ＭＳ ゴシック"));

		SendMessage(hWndInput, WM_SETFONT, (WPARAM)hNewFont, (LPARAM)MAKELPARAM(FALSE, 0));
		SendMessage(hWndList, WM_SETFONT, (WPARAM)hNewFont, (LPARAM)MAKELPARAM(FALSE, 0));
		HWND tmp = SetFocus(GetDlgItem(hwnd, IDC_MACRO_LIST));
		return (INT_PTR)FALSE;
		break;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wp))
		{
		case IDOK:
		{
			HWND hWndCtrl = GetFocus();
			if (hWndCtrl) {
				PostMessage(hwnd, WM_NEXTDLGCTL, 0, 0L);
			}
			return (INT_PTR)TRUE;
			break;
		}
		case IDCANCEL:
		{
			PostMessage(hWndMacro, WM_CLOSE, 0, 0);
			return (INT_PTR)TRUE;
			break;
		}
		case IDC_MACRO_ENTER:
		{
			TCHAR str[MACRO_MAX_LEN];
			TCHAR strbuff[MACRO_MAX_LEN + 3];

			if (g_macroStrings.size() >= 10) {
				MessageBeep(MB_ICONEXCLAMATION);
				break;
			}

			GetWindowText(GetDlgItem(hwnd, IDC_MACRO_INPUT), str, MACRO_MAX_LEN);
			Rtrim(str);
			if (_tcslen(str) != 0) {
				wsprintf(strbuff, L"%d:", (int)g_macroStrings.size() + 1);
				_tcscat(strbuff, str);
				SendMessage(hWndList, LB_ADDSTRING, 0, (LPARAM)strbuff);
				g_macroStrings.push_back(str);
			}
			return (INT_PTR)TRUE;
			break;
		}
		}

		if (HIWORD(wp) == LBN_DBLCLK) {
			int idx = (int)SendMessage(hWndList, LB_GETCURSEL, 0, 0);
			AutoTypeMacro(idx);
		}

		return (INT_PTR)FALSE;
	}
	case WM_CLOSE:
	{

		DestroyWindow(hWndMacro);
		hWndMacro = NULL;
		bMacroWindow = false;
		CheckMenuItem(GetMenu(g_hMainWnd), IDM_TOOL_MACRO, MF_UNCHECKED);
		return (INT_PTR)TRUE;
		break;
	}
	case WM_DESTROY:
	{
		DeleteObject(hNewFont);
		return (INT_PTR)TRUE;
		break;
	}
	}
	return (INT_PTR)FALSE;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバッグウィンドウ　ウォッチ項目削除
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void DeleteSelectedWatchListItem()
{
	HWND hWatchList = GetDlgItem(hWndDebug, IDC_DEBUG_WLIST);

	while (1) {
		int nItem = ListView_GetNextItem(hWatchList, -1, LVNI_ALL | LVNI_SELECTED);
		if (nItem == -1)
			break;
		ListView_DeleteItem(hWatchList, nItem);
		delete watchVarList[nItem];
		watchVarList.erase(watchVarList.begin() + nItem);
	}
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバッグウィンドウ　キーフック　コールバック関数
//
////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK DebugKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (hWndDebug == NULL) 
		return CallNextHookEx(hDebugHook, nCode, wParam, lParam);

	if (nCode < 0)
		return CallNextHookEx(hDebugHook, nCode, wParam, lParam);

	if (wParam == VK_DELETE) {
		DeleteSelectedWatchListItem();
	}

	return CallNextHookEx(hDebugHook, nCode, wParam, lParam);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバッグ・ウィンドウのウィンドウ・プロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK DlgDebugProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static HFONT hNewFont;
	static HWND hWatchList;
	static HWND hJumpList;

	UINT ctlList[] = { IDC_DEBUG_BREAKPOINT, IDC_DEBUG_BREAKPOINT2, IDC_DEBUG_BREAKPOINT3,
		IDC_DEBUG_BREAKPOINT4, IDC_DEBUG_BREAKPOINT5, IDC_DEBUG_BREAKPOINT6,
		IDC_DEBUG_BREAKPOINT7, IDC_DEBUG_BREAKPOINT8, IDC_DEBUG_BREAKPOINT9,
		IDC_DEBUG_BREAKPOINT10, IDC_DEBUG_BREAKPOINT11, IDC_DEBUG_BREAKPOINT12 };

	UINT chkButtonlList[] = { IDC_DEBUG_BPENABLE1, IDC_DEBUG_BPENABLE2, IDC_DEBUG_BPENABLE3,
		IDC_DEBUG_BPENABLE4, IDC_DEBUG_BPENABLE5, IDC_DEBUG_BPENABLE6,
		IDC_DEBUG_BPENABLE7, IDC_DEBUG_BPENABLE8, IDC_DEBUG_BPENABLE9,
		IDC_DEBUG_BPENABLE10, IDC_DEBUG_BPENABLE11, IDC_DEBUG_BPENABLE12 };

	UINT ctlRWBreakList[] = { IDC_DEBUG_RWBREAK1S, IDC_DEBUG_RWBREAK1E,
		IDC_DEBUG_RWBREAK2S, IDC_DEBUG_RWBREAK2E,
		IDC_DEBUG_RWBREAK3S, IDC_DEBUG_RWBREAK3E,
		IDC_DEBUG_RWBREAK4S, IDC_DEBUG_RWBREAK4E,
		IDC_DEBUG_RWBREAK5S, IDC_DEBUG_RWBREAK5E
	};

	UINT chkRWBreakButtonlList[] = { IDC_DEBUG_RWBP_R1, IDC_DEBUG_RWBP_W1, 
		IDC_DEBUG_RWBP_R2, IDC_DEBUG_RWBP_W2,
		IDC_DEBUG_RWBP_R3, IDC_DEBUG_RWBP_W3,
		IDC_DEBUG_RWBP_R4, IDC_DEBUG_RWBP_W4,
		IDC_DEBUG_RWBP_R5, IDC_DEBUG_RWBP_W5
	};

	switch (msg) {
	case WM_INITDIALOG:
	{
		if (sys.pMn1271->debugTcaEnable)
			CheckDlgButton(hwnd, IDC_DEBUG_TCA, BST_CHECKED);
		else
			CheckDlgButton(hwnd, IDC_DEBUG_TCA, BST_UNCHECKED);
		if (sys.pMn1271->debugTcbEnable)
			CheckDlgButton(hwnd, IDC_DEBUG_TCB, BST_CHECKED);
		else
			CheckDlgButton(hwnd, IDC_DEBUG_TCB, BST_UNCHECKED);
		if (sys.pMn1271->debugTccEnable)
			CheckDlgButton(hwnd, IDC_DEBUG_TCC, BST_CHECKED);
		else
			CheckDlgButton(hwnd, IDC_DEBUG_TCC, BST_UNCHECKED);
		if (sys.pMn1271->debugTcdEnable)
			CheckDlgButton(hwnd, IDC_DEBUG_TCD, BST_CHECKED);
		else
			CheckDlgButton(hwnd, IDC_DEBUG_TCD, BST_UNCHECKED);
		if (sys.pMn1271->debugTceEnable)
			CheckDlgButton(hwnd, IDC_DEBUG_TCE, BST_CHECKED);
		else
			CheckDlgButton(hwnd, IDC_DEBUG_TCE, BST_UNCHECKED);
		if (sys.pMn1271->debugTcfEnable)
			CheckDlgButton(hwnd, IDC_DEBUG_TCF, BST_CHECKED);
		else
			CheckDlgButton(hwnd, IDC_DEBUG_TCF, BST_UNCHECKED);

		for (int i = 0; i < BREAKPOINT_NUM; ++i) {
			CheckDlgButton(hwnd, chkButtonlList[i], BST_CHECKED);
		}

		hWatchList = GetDlgItem(hwnd, IDC_DEBUG_WLIST);

		DWORD dwStyle = ListView_GetExtendedListViewStyle(hWatchList);
		dwStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP;
		ListView_SetExtendedListViewStyle(hWatchList, dwStyle);

		LVCOLUMN lvCol;
		lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvCol.fmt = LVCFMT_LEFT;
		lvCol.pszText = _T("Name");
		lvCol.cx = 90;
		ListView_InsertColumn(hWatchList, 0, &lvCol);

		lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvCol.fmt = LVCFMT_RIGHT;
		lvCol.pszText = _T("Hex");
		lvCol.cx = 40;
		ListView_InsertColumn(hWatchList, 1, &lvCol);

		lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvCol.fmt = LVCFMT_RIGHT;
		lvCol.pszText = _T("Dec");
		lvCol.cx = 50;
		ListView_InsertColumn(hWatchList, 2, &lvCol);

		lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvCol.fmt = LVCFMT_RIGHT;
		lvCol.pszText = _T("Size");
		lvCol.cx = 40;
		ListView_InsertColumn(hWatchList, 3, &lvCol);

		lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvCol.fmt = LVCFMT_RIGHT;
		lvCol.pszText = _T("Adrs");
		lvCol.cx = 50;
		ListView_InsertColumn(hWatchList, 4, &lvCol);

		HWND hCombo = GetDlgItem(hwnd, IDC_DEBUG_WBYTES);
		SendMessage(hCombo, CB_INSERTSTRING, 0, (LPARAM)_T("1"));
		SendMessage(hCombo, CB_INSERTSTRING, 1, (LPARAM)_T("2"));
		SendMessage(hCombo, CB_SETCURSEL, (WPARAM)0, 0);

		hNewFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
			_T("Courier New"));
		SendMessage(hWatchList, WM_SETFONT, (WPARAM)hNewFont, (LPARAM)MAKELPARAM(FALSE, 0));

		// ジャンプリスト
		hJumpList = GetDlgItem(hwnd, IDC_DEBUG_JUMPLIST);

		int idx = g_JumpHistory_index;
		if (--idx < 0)
			idx = JUMP_HISTORY_SIZE - 1;
		for (int i = 0; i < JUMP_HISTORY_SIZE; ++i) {
			SendMessage(hJumpList, LB_ADDSTRING, 0, (LPARAM)(g_JumpHistory[idx]));
			if (--idx < 0)
				idx = JUMP_HISTORY_SIZE - 1;
		}
		SendMessage(hJumpList, WM_SETFONT, (WPARAM)hNewFont, (LPARAM)MAKELPARAM(FALSE, 0));

		SetFocus(GetDlgItem(hwnd, IDC_DEBUG_BREAKPOINT));

		hDebugHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)DebugKeyboardProc, NULL, GetCurrentThreadId());
		return (INT_PTR)FALSE;
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wp))
		{
		case IDOK:
		{
			HWND hWndCtrl = GetFocus();
			if (hWndCtrl) {
				PostMessage(hwnd, WM_NEXTDLGCTL, 0, 0L);
				return TRUE;
			}
			break;
		}
		case IDC_DEBUG_PAUSE:
		{
			g_debug = 2;
			g_RWBreak[0] = _T('\0');
			break;
		}
		case IDC_DEBUG_STEP:
		{
			g_debug = 1;
			g_RWBreak[0] = _T('\0');
			break;
		}
		case IDC_DEBUG_PLAY:
		{
			g_debug = -1;
			g_RWBreak[0] = _T('\0');
			break;
		}
		case IDC_DEBUG_CLEAR:
		{

			for (int i = 0; i < BREAKPOINT_NUM; ++i) {
				SetDlgItemText(hwnd, ctlList[i], _T(""));
				g_breakPoint[i] = -1;
			}

			for (int i = 0; i < BREAKPOINT_NUM; ++i) {
				CheckDlgButton(hwnd, chkButtonlList[i], BST_CHECKED);
			}

			break;
		}
		case IDC_DEBUG_SET:
		{
			TCHAR buff[10];

			for (int i = 0; i < BREAKPOINT_NUM; ++i) {
				GetDlgItemText(hwnd, ctlList[i], buff, 9);
				Rtrim(buff);
				TCHAR* e;
				unsigned int address = _tcstol(buff, &e, 16);

				if (*e != '\0' || address < 0 || address > 65535) {
					SetDlgItemText(hwnd, ctlList[i], _T(""));
					MessageBeep(MB_ICONEXCLAMATION);
				}
				else {
					if (IsDlgButtonChecked(hwnd, chkButtonlList[i])) {
						g_breakPoint[i] = address;
					}
					else {
						g_breakPoint[i] = -1;
					}
				}

			}
			break;
		}
		case IDC_DEBUG_RWCLEAR:
		{

			for (int i = 0; i < RW_BREAKPOINT_NUM * 2; ++i) {
				SetDlgItemText(hwnd, ctlRWBreakList[i], _T(""));
				g_rwBreakPoint[i / 2][0] = -1;
				g_rwBreakPoint[i / 2][1] = -1;
			}

			for (int i = 0; i < RW_BREAKPOINT_NUM * 2; ++i) {
				CheckDlgButton(hwnd, chkRWBreakButtonlList[i], BST_UNCHECKED);
			}

			break;
		}
		case IDC_DEBUG_RWSET:
		{
			TCHAR sbuff[10], ebuff[10];
			int s_address, e_address;
			bool isValid = true;

			for (int i = 0; i < RW_BREAKPOINT_NUM * 2; i += 2) {
				GetDlgItemText(hwnd, ctlRWBreakList[i], sbuff, 9);
				Rtrim(sbuff);
				GetDlgItemText(hwnd, ctlRWBreakList[i + 1], ebuff, 9);
				Rtrim(ebuff);

				if (_tcslen(sbuff) == 0 && _tcslen(ebuff) != 0) {
					isValid = false;
				}

				if (isValid) {
					s_address = CheckAddress(sbuff);
					if (s_address == -1) {
						isValid = false;
					}
				}

				if (isValid) {
					e_address = CheckAddress(ebuff);
					if (e_address == -1) {
						isValid = false;
					}
				}

				if (isValid) {
					if (e_address != -2 && (s_address > e_address))
						isValid = false;
				}

				int j = (int)(i / 2);

				if (isValid) {
					g_rwBreakPoint[j][0] = s_address;
					g_rwBreakPoint[j][1] = e_address;
				}
				else {
					SetDlgItemText(hwnd, ctlRWBreakList[i], _T(""));
					SetDlgItemText(hwnd, ctlRWBreakList[i + 1], _T(""));
					int j = (int)(i / 2);
					g_rwBreakPoint[j][0] = -1;
					g_rwBreakPoint[j][1] = -1;
				}


				// チェックボックス
				if (IsDlgButtonChecked(hwnd, chkRWBreakButtonlList[i])) {
					g_rwBKEnableR[j] = true;
				}
				else {
					g_rwBKEnableR[j] = false;
				}
				if (IsDlgButtonChecked(hwnd, chkRWBreakButtonlList[i + 1])) {
					g_rwBKEnableW[j] = true;
				}
				else {
					g_rwBKEnableW[j] = false;
				}
			}

			if (!isValid)
				MessageBeep(MB_ICONEXCLAMATION);

			break;
		}
		case IDC_DEBUG_WENTER:
		{
			TCHAR name[WATCHVAR_NAME_LEN], sizeBuff[10], addressBuff[10], hex[10], dec[10];
			int size, address, val;

			GetDlgItemText(hwnd, IDC_DEBUG_WNAME, name, WATCHVAR_NAME_LEN);
			name[WATCHVAR_NAME_LEN - 1] = _T('\0');
			Rtrim(name);
			if (_tcslen(name) == 0) {
				MessageBeep(MB_ICONEXCLAMATION);
				return TRUE;
			}

			GetDlgItemText(hwnd, IDC_DEBUG_WBYTES, sizeBuff, 10);

			TCHAR* e;
			size = _tcstol(sizeBuff, &e, 10);

			GetDlgItemText(hwnd, IDC_DEBUG_WADDRESS, addressBuff, 10);
			Rtrim(addressBuff);
			if (_tcslen(addressBuff) == 0) {
				MessageBeep(MB_ICONEXCLAMATION);
				return TRUE;
			}

			address = _tcstol(addressBuff, &e, 16);

			if (*e != '\0' || address < 0 || address > 65535) {
				SetDlgItemText(hwnd, IDC_DEBUG_WADDRESS, _T(""));
				MessageBeep(MB_ICONEXCLAMATION);
				return TRUE;
			}

			val = sys.pAddress->ReadByteForDebug(address);
			if (size == 2) {
				val <<= 8;
				val += sys.pAddress->ReadByteForDebug(address + 1);
			}

			LVITEM lvItem;
			lvItem.mask = LVIF_TEXT;
			lvItem.pszText = name;
			lvItem.iItem = (int)watchVarList.size();
			lvItem.iSubItem = 0;
			ListView_InsertItem(hWatchList, &lvItem);

			lvItem.mask = LVIF_TEXT;
			wsprintf(hex, _T("%X"), val);
			lvItem.pszText = hex;
			lvItem.iItem = (int)watchVarList.size();
			lvItem.iSubItem = 1;
			ListView_SetItem(hWatchList, &lvItem);

			lvItem.mask = LVIF_TEXT;
			wsprintf(dec, _T("%d"), val);
			lvItem.pszText = dec;
			lvItem.iItem = (int)watchVarList.size();
			lvItem.iSubItem = 2;
			ListView_SetItem(hWatchList, &lvItem);

			lvItem.mask = LVIF_TEXT;
			lvItem.pszText = sizeBuff;
			lvItem.iItem = (int)watchVarList.size();
			lvItem.iSubItem = 3;
			ListView_SetItem(hWatchList, &lvItem);

			lvItem.mask = LVIF_TEXT;
			wsprintf(addressBuff, _T("%04X"), address);
			lvItem.pszText = addressBuff;
			lvItem.iItem = (int)watchVarList.size();
			lvItem.iSubItem = 4;
			ListView_SetItem(hWatchList, &lvItem);


			WatchVar* var = new WatchVar();
			_tcsncpy(var->name, name, WATCHVAR_NAME_LEN);
			var->name[WATCHVAR_NAME_LEN - 1] = _T('\0');
			var->size = size;
			var->address = address;
			watchVarList.push_back(var);

			break;
		}
		case IDM_DEBUG_CONTEXT_DELETE:
		{
			DeleteSelectedWatchListItem();
			break;
		}

		case IDC_DEBUG_WALLDEL:
		{
			while (1) {
				int nItem = ListView_GetNextItem(hWatchList, -1, LVNI_ALL);
				if (nItem == -1)
					break;
				ListView_DeleteItem(hWatchList, nItem);
			}

			watchVarList.clear();
			break;
		}
		case IDC_DEBUG_TCA:
			if (IsDlgButtonChecked(hwnd, IDC_DEBUG_TCA))
				sys.pMn1271->debugTcaEnable = true;
			else
				sys.pMn1271->debugTcaEnable = false;
			break;
		case IDC_DEBUG_TCB:
			if (IsDlgButtonChecked(hwnd, IDC_DEBUG_TCB))
				sys.pMn1271->debugTcbEnable = true;
			else
				sys.pMn1271->debugTcbEnable = false;
			break;
		case IDC_DEBUG_TCC:
			if (IsDlgButtonChecked(hwnd, IDC_DEBUG_TCC))
				sys.pMn1271->debugTccEnable = true;
			else
				sys.pMn1271->debugTccEnable = false;
			break;
		case IDC_DEBUG_TCD:
			if (IsDlgButtonChecked(hwnd, IDC_DEBUG_TCD))
				sys.pMn1271->debugTcdEnable = true;
			else
				sys.pMn1271->debugTcdEnable = false;
			break;
		case IDC_DEBUG_TCE:
			if (IsDlgButtonChecked(hwnd, IDC_DEBUG_TCE))
				sys.pMn1271->debugTceEnable = true;
			else
				sys.pMn1271->debugTceEnable = false;
			break;
		case IDC_DEBUG_TCF:
			if (IsDlgButtonChecked(hwnd, IDC_DEBUG_TCF))
				sys.pMn1271->debugTcfEnable = true;
			else
				sys.pMn1271->debugTcfEnable = false;
			break;
		case IDC_DEBUG_BPENABLE1:
		case IDC_DEBUG_BPENABLE2:
		case IDC_DEBUG_BPENABLE3:
		case IDC_DEBUG_BPENABLE4:
		case IDC_DEBUG_BPENABLE5:
		case IDC_DEBUG_BPENABLE6:
		case IDC_DEBUG_BPENABLE7:
		case IDC_DEBUG_BPENABLE8:
		case IDC_DEBUG_BPENABLE9:
		case IDC_DEBUG_BPENABLE10:
		case IDC_DEBUG_BPENABLE11:
		case IDC_DEBUG_BPENABLE12:
		{
			PostMessage(hwnd, WM_COMMAND, IDC_DEBUG_SET, 0);

			break;
		}
		case IDC_DEBUG_BREAKPOINT:
		case IDC_DEBUG_BREAKPOINT2:
		case IDC_DEBUG_BREAKPOINT3:
		case IDC_DEBUG_BREAKPOINT4:
		case IDC_DEBUG_BREAKPOINT5:
		case IDC_DEBUG_BREAKPOINT6:
		case IDC_DEBUG_BREAKPOINT7:
		case IDC_DEBUG_BREAKPOINT8:
		case IDC_DEBUG_BREAKPOINT9:
		case IDC_DEBUG_BREAKPOINT10:
		case IDC_DEBUG_BREAKPOINT11:
		case IDC_DEBUG_BREAKPOINT12:
		{
			if ( HIWORD(wp) == EN_KILLFOCUS) {
				PostMessage(hwnd, WM_COMMAND, IDC_DEBUG_SET, 0);
			}
			break;
		}
		case IDC_DEBUG_RWBP_R1:
		case IDC_DEBUG_RWBP_W1:
		case IDC_DEBUG_RWBP_R2:
		case IDC_DEBUG_RWBP_W2:
		case IDC_DEBUG_RWBP_R3:
		case IDC_DEBUG_RWBP_W3:
		case IDC_DEBUG_RWBP_R4:
		case IDC_DEBUG_RWBP_W4:
		case IDC_DEBUG_RWBP_R5:
		case IDC_DEBUG_RWBP_W5: 
		{
			PostMessage(hwnd, WM_COMMAND, IDC_DEBUG_RWSET, 0);
			break;
		}
		case IDC_DEBUG_RWBREAK1S:
		case IDC_DEBUG_RWBREAK1E:
		case IDC_DEBUG_RWBREAK2S:
		case IDC_DEBUG_RWBREAK2E:
		case IDC_DEBUG_RWBREAK3S:
		case IDC_DEBUG_RWBREAK3E:
		case IDC_DEBUG_RWBREAK4S:
		case IDC_DEBUG_RWBREAK4E:
		case IDC_DEBUG_RWBREAK5S:
		case IDC_DEBUG_RWBREAK5E:
		{
			if (HIWORD(wp) == EN_KILLFOCUS) {
				PostMessage(hwnd, WM_COMMAND, IDC_DEBUG_RWSET, 0);
			}
			break;
		}
		}
		return (INT_PTR)TRUE;
		break;
	}
	case WM_CLOSE:
	{
		DestroyWindow(hWndDebug);
		CheckMenuItem(GetMenu(g_hMainWnd), IDM_TOOL_DEBUGWINDOW, MF_UNCHECKED);
		return (INT_PTR)TRUE;
	}
	case WM_DESTROY:
	{
		g_debug = -1;
		watchVarList.clear();

		UnhookWindowsHookEx(hDebugHook);
		hDebugHook = NULL;

		DeleteObject(hNewFont);
		hWndDebug = NULL;
		bDebugWindow = false;

		return (INT_PTR)TRUE;
		break;
	}
	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lp)->idFrom)
		{
		case IDC_DEBUG_WLIST:
			switch (((LPNMLISTVIEW)lp)->hdr.code)
			{
			case NM_RCLICK:
			{
				LV_HITTESTINFO lvinfo;
				int x, y;
				HMENU hmenu, hSubmenu;
				HWND hWndList = GetDlgItem(hWndDebug, IDC_DEBUG_WLIST);

				GetCursorPos((LPPOINT)&lvinfo.pt);
				x = lvinfo.pt.x;
				y = lvinfo.pt.y;
				ScreenToClient(((LPNMLISTVIEW)lp)->hdr.hwndFrom, &lvinfo.pt);

				ListView_HitTest(((LPNMLISTVIEW)lp)->hdr.hwndFrom, &lvinfo);
				if ((lvinfo.flags & LVHT_ONITEM) != 0)
				{
					ListView_SetItemState(hWndList, lvinfo.iItem, LVIS_SELECTED, LVIS_SELECTED);

					hmenu = LoadMenu((HINSTANCE)g_hMod, MAKEINTRESOURCE(IDR_DEBUG_CONTEXT));
					hSubmenu = GetSubMenu(hmenu, 0);
					TrackPopupMenu(hSubmenu, TPM_LEFTALIGN, x, y, 0, hWndDebug, NULL);
					DestroyMenu(hmenu);
				}
				break;
			}
			}
		}
		return (INT_PTR)TRUE;
		break;
	}
	case WM_PAINT:
	{
		const int OFFSET = 40;
		TCHAR c[40] = {};
		uint16_t pc, sp, x, ppc;
		uint8_t a, b, cc;
		sys.pCpu->GetRegister(pc, sp, x, a, b, cc, ppc);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		HFONT hFont, hFontOld;

		Rectangle(hdc, 5, 5, 265, 75);
		Rectangle(hdc, 5, 80, 265, 260);

		SetTextColor(hdc, RGB(0, 0, 0));
		SetBkColor(hdc, RGB(255, 255, 255));

		hFont = CreateFont(17,
			0,
			0,
			0,
			FW_BOLD,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			PROOF_QUALITY,
			FIXED_PITCH | FF_MODERN,
			_T("Courier New"));

		hFontOld = (HFONT)SelectObject(hdc, hFont);

		if (_tcslen(g_RWBreak) != 0){
			TCHAR s[10];
			wsprintf(s, _T("%04X"), ppc);
			TextOut(hdc, 460, 50, s, (int)_tcslen(s));
			TextOut(hdc, 460, 100, g_RWBreak, (int)_tcslen(g_RWBreak));
		}

		wsprintf(c, _T("PC=%04X   SP=%04X"), pc, sp);
		TextOut(hdc, OFFSET, 10, c, (int)_tcslen(c));
		wsprintf(c, _T("A=%02X   B=%02X   X=%04X"), a, b, x);
		TextOut(hdc, OFFSET, 25, c, (int)_tcslen(c));
		wsprintf(c, _T("CC - - H I N Z V C"));
		TextOut(hdc, OFFSET, 40, c, (int)_tcslen(c));
		wsprintf(c, _T("%02X 1 1 %d %d %d %d %d %d"), cc, (cc & 0x20) ? 1 : 0, (cc & 0x10) ? 1 : 0, (cc & 0x08) ? 1 : 0,
			(cc & 0x04) ? 1 : 0, (cc & 0x02) ? 1 : 0, (cc & 0x01) ? 1 : 0);
		TextOut(hdc, OFFSET, 55, c, (int)_tcslen(c));

		for (int i = 0; i < 32; ++i) {
			ioReg[i] = sys.pAddress->ReadByteForDebug(0xc800 + i);
		}
		wsprintf(c, _T("MN1271"));
		TextOut(hdc, 10, 85, c, (int)_tcslen(c));

		wsprintf(c, _T("REG:00 01 02 03 04 05 06 07"));
		TextOut(hdc, 10, 100, c, (int)_tcslen(c));
		wsprintf(c, _T("VAL:%02X %02X %02X %02X %02X %02X %02X %02X"), ioReg[0], ioReg[1], ioReg[2], ioReg[3], ioReg[4], ioReg[5], ioReg[6], ioReg[7]);
		TextOut(hdc, 10, 115, c, (int)_tcslen(c));

		wsprintf(c, _T("REG:08 09 0A 0B 0C 0D 0E 0F"));
		TextOut(hdc, 10, 140, c, (int)_tcslen(c));
		wsprintf(c, _T("VAL:%02X %02X %02X %02X %02X %02X %02X %02X"), ioReg[8], ioReg[9], ioReg[10], ioReg[11], ioReg[12], ioReg[13], ioReg[14], ioReg[15]);
		TextOut(hdc, 10, 155, c, (int)_tcslen(c));

		wsprintf(c, _T("REG:10 11 12 13 14 15 16 17"));
		TextOut(hdc, 10, 180, c, (int)_tcslen(c));
		wsprintf(c, _T("VAL:%02X %02X %02X %02X %02X %02X %02X %02X"), ioReg[16], ioReg[17], ioReg[18], ioReg[19], ioReg[20], ioReg[21], ioReg[22], ioReg[23]);
		TextOut(hdc, 10, 195, c, (int)_tcslen(c));

		wsprintf(c, _T("REG:18 19 1A 1B 1C 1D 1E 1F"));
		TextOut(hdc, 10, 220, c, (int)_tcslen(c));
		wsprintf(c, _T("VAL:%02X %02X %02X %02X %02X %02X %02X %02X"), ioReg[24], ioReg[25], ioReg[26], ioReg[27], ioReg[28], ioReg[29], ioReg[30], ioReg[31]);
		TextOut(hdc, 10, 235, c, (int)_tcslen(c));

		SelectObject(hdc, hFontOld);
		DeleteObject(hFont);
		EndPaint(hwnd, &ps);

		return (INT_PTR)TRUE;
		break;
	}
	}
	return (INT_PTR)FALSE;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ROM,FONTファイル設定ダイアログのウィンドウ・プロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK DlgRomFontProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_ROMFILE, g_pRomFile);
		SetDlgItemText(hDlg, IDC_FONTFILE, g_pFontFile);
		SetDlgItemText(hDlg, IDC_FDDROMFILE, g_pFdRomFile);
		return (INT_PTR)TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			TCHAR buff[MAX_PATH];
			int len;

			GetDlgItemText(hDlg, IDC_ROMFILE, buff, MAX_PATH);
			len = (int)_tcslen(buff);

			if (len == 0) {
				MessageBox(hDlg, g_strTable[(int)Msg::Please_specify_the_file], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
				return (INT_PTR)TRUE;
			}

			HANDLE hFile;
			hFile = CreateFile(buff,
				GENERIC_READ,
				0,
				0,
				OPEN_EXISTING,
				0,
				0
			);
			if (hFile == INVALID_HANDLE_VALUE) {
				CloseHandle(hFile);
				MessageBox(hDlg, g_strTable[(int)Msg::The_specified_ROM_file_can_not_be_opened], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
				return (INT_PTR)TRUE;
			}

			if (GetFileSize(hFile, NULL) != 16384) {
				CloseHandle(hFile);
				MessageBox(hDlg, g_strTable[(int)Msg::The_size_of_the_specified_ROM_file_is_incorrect], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
				return (INT_PTR)TRUE;
			}
			CloseHandle(hFile);
			_tcscpy(g_pRomFile, buff);

			// FONTファイル
			GetDlgItemText(hDlg, IDC_FONTFILE, buff, MAX_PATH);
			len = (int)_tcslen(buff);

			if (len == 0) {
				MessageBox(hDlg, g_strTable[(int)Msg::Please_specify_the_file], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
				return (INT_PTR)TRUE;
			}

			hFile = CreateFile(buff,
				GENERIC_READ,
				0,
				0,
				OPEN_EXISTING,
				0,
				0
			);
			if (hFile == INVALID_HANDLE_VALUE) {
				CloseHandle(hFile);
				MessageBox(hDlg, g_strTable[(int)Msg::The_specified_FONT_file_can_not_be_opened], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
				return (INT_PTR)TRUE;
			}

			if (GetFileSize(hFile, NULL) != 2048) {
				CloseHandle(hFile);
				MessageBox(hDlg, g_strTable[(int)Msg::The_size_of_the_specified_FONT_file_is_incorrect], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
				return (INT_PTR)TRUE;
			}
			CloseHandle(hFile);
			_tcscpy(g_pFontFile, buff);


			// FDD拡張ROMファイル　このファイルは任意
			GetDlgItemText(hDlg, IDC_FDDROMFILE, buff, MAX_PATH);
			len = (int)_tcslen(buff);
			g_bFddEnabled = false;

			if (len != 0) {
				hFile = CreateFile(buff,
					GENERIC_READ,
					0,
					0,
					OPEN_EXISTING,
					0,
					0
				);
				if (hFile == INVALID_HANDLE_VALUE) {
					CloseHandle(hFile);
					MessageBox(hDlg, g_strTable[(int)Msg::The_specified_ROM_file_can_not_be_opened], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
					_tcscpy(g_pFdRomFile, _T(""));
				}
				else {
					if (GetFileSize(hFile, NULL) != 2048) {
						CloseHandle(hFile);
						MessageBox(hDlg, g_strTable[(int)Msg::The_size_of_the_specified_ROM_file_is_incorrect], g_strTable[(int)Msg::Caution], MB_OK | MB_ICONEXCLAMATION);
						_tcscpy(g_pFdRomFile, _T(""));
					}
					else {
						CloseHandle(hFile);
						_tcscpy(g_pFdRomFile, buff);
					}
				}
			}
			else {
				_tcscpy(g_pFdRomFile, _T(""));
			}

			settingXml.Write();
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		case IDC_ROMFONT_ROMFILEOPEN:
		{
			TCHAR filename[MAX_PATH] = {};
			if (OpenFileDialog(_T("ROM file(*.*)\0*.*\0"), filename, hDlg))
				SetDlgItemText(hDlg, IDC_ROMFILE, filename);
			return (INT_PTR)TRUE;
			break;
		}
		case IDC_ROMFONT_FONTFILEOPEN:
		{
			TCHAR filename[MAX_PATH] = {};
			if (OpenFileDialog(_T("Font flie(*.*)\0*.*\0"), filename, hDlg))
				SetDlgItemText(hDlg, IDC_FONTFILE, filename);
			return (INT_PTR)TRUE;
			break;
		}
		case IDC_ROMFONT_FDDROMFILEOPEN:
		{
			TCHAR filename[MAX_PATH] = {};
			if (OpenFileDialog(_T("ROM flie(*.*)\0*.*\0"), filename, hDlg))
				SetDlgItemText(hDlg, IDC_FDDROMFILE, filename);
			return (INT_PTR)TRUE;
			break;
		}
		}
		break;
	}
	return (INT_PTR)FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ファイル・オープン・ダイアログの共通ルーチン
//
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL OpenFileDialog(TCHAR* filter, TCHAR* fileName, HWND hwnd)
{
	OPENFILENAME ofn = { };

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = filter;
	ofn.nMaxCustFilter = 256;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST;

	return GetOpenFileName(&ofn);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FPS表示タイマのコールバック・ルーチン
//
////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CALLBACK FpsTimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	TCHAR c[40];
	wsprintf(c, _T("VJR-200        %dFPS   CLOCK %d %%"), fps, g_cpuScale);
	SetWindowText(g_hMainWnd, c);
	fps = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウ・サイズが変更された時の処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void ChangeWindowSize(HMENU hMenu)
{
	int mes = 0;
	int statusbarHeight = g_bStatusBar ? STATUSBAR_HEIGHT : 0;

	switch (g_viewScale)
	{
	case 1:
		mes = IDM_VIEW_X1;
		break;
	case 2:
		mes = IDM_VIEW_X2;
		break;
	case 3:
		mes = IDM_VIEW_X3;
		break;
	case 4:
		mes = IDM_VIEW_X4;
		break;
	case 5:
		mes = IDM_VIEW_X5;
		break;
	}
	int client_h = BITMAP_H * g_viewScale;
	int client_w = BITMAP_W * g_viewScale;

	if (g_rotate == 90 || g_rotate == 270) {
		int tmp = client_h;
		client_h = client_w;
		client_w = tmp;
	}

	SetMenuItemState(hMenu);

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	RECT rect;
	if (g_bSquarePixel)
		rect = { 0, 0, client_w, client_h + statusbarHeight };
	else {
		if (g_rotate == 0 || g_rotate == 180)
			rect = { 0, 0, (int)(client_w * REAL_WRATIO), client_h + statusbarHeight };
		else 
			rect = { 0, 0, client_w, (int)(client_h * REAL_WRATIO) + statusbarHeight };
	}
	
	AdjustWindowRect(&rect, style, TRUE);
	SetWindowPos(g_hMainWnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// メニュー内容が変更された時の処理
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetMenuItemState(HMENU hMenu)
{
	switch (g_viewScale)
	{
	case 1:
		CheckMenuRadioItem(hMenu, IDM_VIEW_X1, IDM_VIEW_X5, IDM_VIEW_X1, MF_BYCOMMAND);
		break;
	case 2:
		CheckMenuRadioItem(hMenu, IDM_VIEW_X1, IDM_VIEW_X5, IDM_VIEW_X2, MF_BYCOMMAND);
		break;
	case 3:
		CheckMenuRadioItem(hMenu, IDM_VIEW_X1, IDM_VIEW_X5, IDM_VIEW_X3, MF_BYCOMMAND);
		break;
	case 4:
		CheckMenuRadioItem(hMenu, IDM_VIEW_X1, IDM_VIEW_X5, IDM_VIEW_X4, MF_BYCOMMAND);
		break;
	case 5:
		CheckMenuRadioItem(hMenu, IDM_VIEW_X1, IDM_VIEW_X5, IDM_VIEW_X5, MF_BYCOMMAND);
		break;
	}

	if (g_bSquarePixel) {
		CheckMenuRadioItem(hMenu, IDM_VIEW_SQUAREPIXEL, IDM_VIEW_REAL, IDM_VIEW_SQUAREPIXEL, MF_BYCOMMAND);
	}
	else {
		CheckMenuRadioItem(hMenu, IDM_VIEW_SQUAREPIXEL, IDM_VIEW_REAL, IDM_VIEW_REAL, MF_BYCOMMAND);
	}


	if (g_bSmoothing) {
		CheckMenuItem(hMenu, IDM_VIEW_SMOOTHING, MF_CHECKED);
	}
	else {
		CheckMenuItem(hMenu, IDM_VIEW_SMOOTHING, MF_UNCHECKED);
	}

	switch (g_rotate)
	{
	case 0:
		CheckMenuRadioItem(hMenu, IDM_VIEW_R0, IDM_VIEW_R270, IDM_VIEW_R0, MF_BYCOMMAND);
		break;
	case 90:
		CheckMenuRadioItem(hMenu, IDM_VIEW_R0, IDM_VIEW_R270, IDM_VIEW_R90, MF_BYCOMMAND);
		break;
	case 180:
		CheckMenuRadioItem(hMenu, IDM_VIEW_R0, IDM_VIEW_R270, IDM_VIEW_R180, MF_BYCOMMAND);
		break;
	case 270:
		CheckMenuRadioItem(hMenu, IDM_VIEW_R0, IDM_VIEW_R270, IDM_VIEW_R270, MF_BYCOMMAND);
		break;
	default:
		break;
	}

	if (g_bFpsCpu) {
		CheckMenuItem(hMenu, IDM_TOOL_FPSCPU, MF_CHECKED);
	}
	else {
		CheckMenuItem(hMenu, IDM_TOOL_FPSCPU, MF_UNCHECKED);
	}

	if (g_bRomajiKana) {
		CheckMenuItem(hMenu, IDM_TOOL_ROMAJIKANA, MF_CHECKED);
	}
	else {
		CheckMenuItem(hMenu, IDM_TOOL_ROMAJIKANA, MF_UNCHECKED);
	}

	if (g_bPrinterLed) {
		CheckMenuItem(hMenu, IDM_VIEW_PRINTERLED, MF_CHECKED);
	}
	else {
		CheckMenuItem(hMenu, IDM_VIEW_PRINTERLED, MF_UNCHECKED);
	}

	if (g_bStatusBar) {
		CheckMenuItem(hMenu, IDM_VIEW_STATUSBAR, MF_CHECKED);
	}
	else {
		CheckMenuItem(hMenu, IDM_VIEW_STATUSBAR, MF_UNCHECKED);
	}

	if (g_bDetachFdd) {
		CheckMenuItem(hMenu, IDM_FDD_DETACHFDD, MF_CHECKED);
	}
	else {
		CheckMenuItem(hMenu, IDM_FDD_DETACHFDD, MF_UNCHECKED);
	}

	SetMenuItemForRecentFiles();
	SetMenuItemForStateSaveLoad();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// リフレッシュレートの取得（正常に取得できなけれは60Hzに設定）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int GetRefreshRate()
{
	int ret;

	HDC hdc = GetDC(g_hMainWnd);
	ret = GetDeviceCaps(hdc, VREFRESH);
	ReleaseDC(g_hMainWnd, hdc);
	if (ret == 0 || ret == 1)
		ret = 60;

	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド再生用のスレッド作成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CreateSoundThreads()
{
	bool val = true;

	unsigned int dwThreadId;
	if (hSoundThread0 == NULL)
		hSoundThread0 = (HANDLE)_beginthreadex(NULL, 0, &SoundSetThread0, NULL, 0, &dwThreadId);
	if (hSoundThread0 == NULL) val = false;

	if (hSoundThread1 == NULL)
		hSoundThread1 = (HANDLE)_beginthreadex(NULL, 0, &SoundSetThread1, NULL, 0, &dwThreadId);
	if (hSoundThread1 == NULL) val = false;

	if (hSoundThread2 == NULL)
		hSoundThread2 = (HANDLE)_beginthreadex(NULL, 0, &SoundSetThread2, NULL, 0, &dwThreadId);
	if (hSoundThread2 == NULL) val = false;

	return val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド再生用のスレッド削除
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void DeleteSoundThreads()
{
	int i;

	if (hSoundThread0 != NULL)
		i = CloseHandle(hSoundThread0);
	if (hSoundThread1 != NULL)
		i = CloseHandle(hSoundThread1);
	if (hSoundThread2 != NULL)
		i = CloseHandle(hSoundThread2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド再生用のスレッド実行ルーチン(0)
//
////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned __stdcall SoundSetThread0(void *p)
{
	for (int i = 0; i < SOUNDBUFFER_BLOCK_NUM - 1; ++i)
		sys.pMn1271->SoundDataCopy(0, i);

	while (true) {
		DWORD i = WaitForMultipleObjects(6, g_hEvent[0], FALSE, INFINITE);
		switch (i) {
		case WAIT_OBJECT_0:
			sys.pMn1271->SoundDataCopy(0, 4);
			break;
		case WAIT_OBJECT_0 + 1:
			sys.pMn1271->SoundDataCopy(0, 0);
			break;
		case WAIT_OBJECT_0 + 2:
			sys.pMn1271->SoundDataCopy(0, 1);
			break;
		case WAIT_OBJECT_0 + 3:
			sys.pMn1271->SoundDataCopy(0, 2);
			break;
		case WAIT_OBJECT_0 + 4:
			sys.pMn1271->SoundDataCopy(0, 3);
			break;
		case WAIT_OBJECT_0 + SOUNDBUFFER_BLOCK_NUM:
		default:
			g_bSoundOn = false;
			_endthreadex(0);
			break;
		}
	}
	return 0L;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド再生用のスレッド実行ルーチン(1)
//
////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned __stdcall SoundSetThread1(void *p)
{
	for (int i = 0; i < SOUNDBUFFER_BLOCK_NUM - 1; ++i)
		sys.pMn1271->SoundDataCopy(1, i);

	while (true) {
		DWORD i = WaitForMultipleObjects(6, g_hEvent[1], FALSE, INFINITE);
		switch (i) {
		case WAIT_OBJECT_0:
			sys.pMn1271->SoundDataCopy(1, 4);
			break;
		case WAIT_OBJECT_0 + 1:
			sys.pMn1271->SoundDataCopy(1, 0);
			break;
		case WAIT_OBJECT_0 + 2:
			sys.pMn1271->SoundDataCopy(1, 1);
			break;
		case WAIT_OBJECT_0 + 3:
			sys.pMn1271->SoundDataCopy(1, 2);
			break;
		case WAIT_OBJECT_0 + 4:
			sys.pMn1271->SoundDataCopy(1, 3);
			break;
		case WAIT_OBJECT_0 + SOUNDBUFFER_BLOCK_NUM:
		default:
			g_bSoundOn = false;
			_endthreadex(0);
			break;
		}
	}
	return 0L;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// サウンド再生用のスレッド実行ルーチン(2)
//
////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned __stdcall SoundSetThread2(void *p)
{
	for (int i = 0; i < SOUNDBUFFER_BLOCK_NUM - 1; ++i)
		sys.pMn1271->SoundDataCopy(2, i);

	while (true) {
		DWORD i = WaitForMultipleObjects(6, g_hEvent[2], FALSE, INFINITE);
		switch (i) {
		case WAIT_OBJECT_0:
			sys.pMn1271->SoundDataCopy(2, 4);
			break;
		case WAIT_OBJECT_0 + 1:
			sys.pMn1271->SoundDataCopy(2, 0);
			break;
		case WAIT_OBJECT_0 + 2:
			sys.pMn1271->SoundDataCopy(2, 1);
			break;
		case WAIT_OBJECT_0 + 3:
			sys.pMn1271->SoundDataCopy(2, 2);
			break;
		case WAIT_OBJECT_0 + 4:
			sys.pMn1271->SoundDataCopy(2, 3);
			break;
		case WAIT_OBJECT_0 + SOUNDBUFFER_BLOCK_NUM:
		default:
			g_bSoundOn = false;
			_endthreadex(0);
			break;
		}
	}
	return 0L;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 「最近使ったファイル」の追加（既にリストにあるファイルは並べ替え）
//
// mode : 0 = CJR,JR2 Set, 1 = Quick Load
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void AddRecentFiles(const TCHAR* str,  int mode)
{
	TCHAR tmpBuf[MAX_PATH];
	tmpBuf[0] = '\0';
	_tcscpy(tmpBuf, str);

	switch (mode)
	{
	case 0: // CJR,JR2 Set
		for (unsigned int i = 0; i < g_rFilesforCJRSet.size(); ++i) {
			if (_tcsicmp(g_rFilesforCJRSet[i].data(), tmpBuf) == 0) {
				g_rFilesforCJRSet.erase(g_rFilesforCJRSet.begin() + i);
			}
		}
		g_rFilesforCJRSet.push_front(tmpBuf);
		if (g_rFilesforCJRSet.size() >= RECENT_FILES_NUM + 1) 
			while (g_rFilesforCJRSet.size() > RECENT_FILES_NUM) g_rFilesforCJRSet.pop_back();
		break;
	case 1: // Quick Load
		for (unsigned int i = 0; i < g_rFilesforQLoad.size(); ++i) {
			if (_tcsicmp(g_rFilesforQLoad[i].data(), tmpBuf) == 0) {
				g_rFilesforQLoad.erase(g_rFilesforQLoad.begin() + i);
			}
		}
		g_rFilesforQLoad.push_front(tmpBuf);
		if (g_rFilesforQLoad.size() >= RECENT_FILES_NUM + 1) 
			while (g_rFilesforQLoad.size() > RECENT_FILES_NUM) g_rFilesforQLoad.pop_back();
		break;
	}

	SetMenuItemForRecentFiles();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 「ステートセーブ&ロード」するファイル名を作成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GetStateFilePathName(TCHAR pathName[], int idx)
{
	TCHAR path[MAX_PATH];

	if (g_pTapeFormat == nullptr || (!g_bDetachFdd && g_bFddEnabled)) {
		SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, path);
		_tcscat(path, APPDATA_PATH);
		_tcscat(path, _T("nomount"));
	}
	else {
		_tcscpy(path, g_pTapeFormat->GetFileName());
	}

	wsprintf(pathName, _T("%s_%d.sta"), path, idx);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 「ステートセーブ&ロード」をメニューに反映
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetMenuItemForStateSaveLoad()
{
	HMENU hMenu, hFileMenu, hSubMenuSave, hSubMenuLoad;
	MENUITEMINFO miiSave, miiLoad;
	TCHAR pathFname[MAX_PATH];
	hMenu = GetMenu(g_hMainWnd);
	hFileMenu = GetSubMenu(hMenu, 0);
	hSubMenuSave = GetSubMenu(hFileMenu, STATESAVE_SUBMENU_POS);
	hSubMenuLoad = GetSubMenu(hFileMenu, STATELOAD_SUBMENU_POS);

	for (unsigned int i = 0; i < 10; ++i) {
		GetStateFilePathName(pathFname, i);
		bool bFileExist = false;

		if (PathFileExists(pathFname)) {
			bFileExist = true;
		}
		TCHAR buf[100];
		miiSave.dwTypeData = buf;
		miiSave.cbSize = sizeof(MENUITEMINFO);
		miiSave.fMask = MIIM_STATE;
		GetMenuItemInfo(hSubMenuSave, IDM_FILE_STATESAVE0 + i, FALSE, &miiSave);
		miiSave.fMask = MIIM_STRING;

		miiLoad.dwTypeData = buf;
		miiLoad.cbSize = sizeof(MENUITEMINFO);
		miiLoad.fMask = MIIM_STATE;
		GetMenuItemInfo(hSubMenuLoad, IDM_FILE_STATELOAD0 + i, FALSE, &miiLoad);
		miiLoad.fMask = MIIM_STRING;

		int idx = i + 1;
		if (idx == 10) idx = 0;

		HANDLE hFile;
		hFile = CreateFile(pathFname,
			GENERIC_READ,
			0,
			0,
			OPEN_EXISTING,
			0,
			0
		);

		FILETIME writeTime, localTime;
		SYSTEMTIME sTime;
		if (hFile == INVALID_HANDLE_VALUE) {
			bFileExist = false;
		}
		else {
			if (!GetFileTime(hFile, NULL, NULL, &writeTime)) {
				bFileExist = false;
			}
		}
		if (hFile != NULL)
			CloseHandle(hFile);

		TCHAR date[50], time[50];
		if (bFileExist) {
			FileTimeToLocalFileTime(&writeTime, &localTime);
			FileTimeToSystemTime(&localTime, &sTime);
			GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, LOCALE_USE_CP_ACP, &sTime, NULL, date, 50, 0);
			GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, LOCALE_USE_CP_ACP, &sTime, NULL, time, 50);
		}

		TCHAR saveText[100], loadText[100];
		if (bFileExist) {
			wsprintf(saveText, _T("&%d. %s  %s\tAlt+%d"), idx, date, time, idx);
			wsprintf(loadText, _T("&%d. %s  %s\tCtrl+Alt+Shift+%d"), idx, date, time, idx);
		}
		else {
			wsprintf(saveText, _T("&%d. ----------\tAlt+%d"), idx, idx);
			wsprintf(loadText, _T("&%d. ----------"), idx);
		}
		miiSave.dwTypeData = saveText;
		miiSave.cch = (int)_tcslen(saveText);
		miiLoad.dwTypeData = loadText;
		miiLoad.cch = (int)_tcslen(loadText);

		SetMenuItemInfo(hSubMenuSave, IDM_FILE_STATESAVE0 + i, FALSE, &miiSave);
		SetMenuItemInfo(hSubMenuLoad, IDM_FILE_STATELOAD0 + i, FALSE, &miiLoad);
		ZeroMemory(&miiSave, sizeof(miiSave));
		ZeroMemory(&miiLoad, sizeof(miiLoad));
	}
	DrawMenuBar(g_hMainWnd);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 文字列の一部を抽出
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Substring(TCHAR* source, int start, int length, TCHAR* destination)
{
	_tcsncpy(destination, source + start, length);
	destination[length] = _T('\0'); // 終端文字を追加
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 「最近使ったファイル」をメニューに反映
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetMenuItemForRecentFiles()
{
	HMENU hMenu, hFileMenu, hRecentFileMenu;
	MENUITEMINFO mii;
	TCHAR buf[MAX_PATH] = {};
	TCHAR pathBuff[MAX_PATH] = {}, pathSub1[30] = {}, pathSub2[30] = {};
	hMenu = GetMenu(g_hMainWnd);
	hFileMenu = GetSubMenu(hMenu, 0);
	hRecentFileMenu = GetSubMenu(hFileMenu, RFILES_SUBMENU_POS);

	// マウント
	for (unsigned int i = 0; i < g_rFilesforCJRSet.size(); ++i) {
		_tcscpy(pathBuff, g_rFilesforCJRSet[i].data());
		TCHAR* fname;
		fname = PathFindFileName(g_rFilesforCJRSet[i].data());
		mii.dwTypeData = buf;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STATE;
		GetMenuItemInfo(hRecentFileMenu, IDM_FILE_RFILES_SETCJR0 + i, FALSE, &mii);

		mii.fMask = MIIM_STRING;
		int idx = i + 1;
		if (idx == 10) idx = 0;

		if (_tcslen(fname) == 0) {
			wsprintf(buf, _T("&%d.%s\tCtrl+Alt+%d"), idx, _T("-----"), idx);
		}
		else {
			PathRemoveFileSpec(pathBuff);
			if (_tcslen(pathBuff) <= 20) {
				wsprintf(buf, _T("&%d.%s%s%s\tCtrl+Alt+%d"), idx, pathBuff, _T("\\"), fname, idx);
			}
			else {
				Substring(pathBuff, 0, 10, pathSub1);
				Substring(pathBuff,(int)(_tcslen(pathBuff) - 10) , 10, pathSub2);
				wsprintf(buf, _T("&%d.%s%s%s%s%s\tCtrl+Alt+%d"), 
					idx, pathSub1, _T("..."), pathSub2, _T("\\"), fname, idx);
			}
		}

		mii.dwTypeData = buf;
		SetMenuItemInfo(hRecentFileMenu, IDM_FILE_RFILES_SETCJR0 + i, FALSE, &mii);
		ZeroMemory(&mii, sizeof(mii));
	}

	// 高速ロード
	for (unsigned int i = 0; i < g_rFilesforQLoad.size(); ++i) {
		_tcscpy(pathBuff, g_rFilesforQLoad[i].data());
		TCHAR* fname;
		TCHAR head_array[] = _T("abcdefghij");
		TCHAR head[] = _T("a");
		fname = PathFindFileName(g_rFilesforQLoad[i].data());
		mii.dwTypeData = buf;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STATE;
		GetMenuItemInfo(hRecentFileMenu, IDM_FILE_RFILES_QLOAD0 + i, FALSE, &mii);

		mii.fMask = MIIM_STRING;
		int idx = i + 1;
		if (idx == 10) idx = 0;

		head[0] = head_array[i];
		if (_tcslen(fname) == 0) {
			wsprintf(buf, _T("&%s.%s\tShift+Alt+%d"), head, _T("-----"), idx);
		}
		else {
			PathRemoveFileSpec(pathBuff);
			if (_tcslen(pathBuff) <= 20) {
				wsprintf(buf, _T("&%s.%s%s%s\tShift+Alt+%d"), head, pathBuff, _T("\\"), fname, idx);
			}
			else {
				Substring(pathBuff, 0, 10, pathSub1);
				Substring(pathBuff, (int)(_tcslen(pathBuff) - 10), 10, pathSub2);
				wsprintf(buf, _T("&%s.%s%s%s%s%s\tShift+Alt+%d"),
					head, pathSub1, _T("..."), pathSub2, _T("\\"), fname, idx);
			}
		}

		mii.dwTypeData = buf;
		SetMenuItemInfo(hRecentFileMenu, IDM_FILE_RFILES_QLOAD0 + i, FALSE, &mii);
		ZeroMemory(&mii, sizeof(mii));
	}

	DrawMenuBar(g_hMainWnd);
	settingXml.Write();
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CJR, JR2フォーマットの判別
//
// 0:エラー　　1:CJR　　2:JR2
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CheckFileFormat(const TCHAR* fileName)
{
	int ret = 0;
	FILE* fp;
	long fileSize;
	char buf[4];

	struct _stat stbuf;
	int st = _tstat(fileName, &stbuf);
	if (st != 0) return 0;
	fileSize = stbuf.st_size;

	TCHAR chkStr[MAX_PATH];
	_tcscpy(chkStr, fileName);
	_tcsupr(chkStr);

	if (_tcsncmp(_tcsrev(chkStr), _T("RJC."), 3) == 0) {
		// CJR
		if (fileSize > CJR_FILE_MAX) return 0;
		fp = _tfopen(fileName, _T("rb"));
		if (fp == nullptr)
			return ret;
		fread(buf, sizeof(uint8_t), 2, fp);
		fclose(fp);
		char head[] = {0x2, 0x2a};
		if (!strncmp(buf, head, 2))
			ret = 1;
	}
	else if (_tcsncmp(chkStr, _T("2RJ."), 3) == 0) {
		// JR2
		fp = _tfopen(fileName, _T("rb"));
		if (fp == nullptr)
			return ret;
		fread(buf, sizeof(uint8_t), 3, fp);
		fclose(fp);
		const char* head = "JR2";
		if (!strncmp(buf, head, 3))
			ret = 2;
	}

	return ret;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ディスクフォーマットの判別
//
// 0:エラー　　1:D88
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CheckDiskFormat(const TCHAR* fileName)
{
	uint32_t fileSize;
	int result = 0;

	struct _stat stbuf;
	int st = _tstat(fileName, &stbuf);
	if (st != 0) return 0;
	fileSize = stbuf.st_size;

	TCHAR chkStr[MAX_PATH];
	_tcscpy(chkStr, fileName);
	_tcsupr(chkStr);

	if (_tcsncmp(_tcsrev(chkStr), _T("02D."), 3) != 0) {
		return 0;
	}
	else {
		if (fileSize > D88_FILE_MAX)
			return 0;
		else
			return 1;
	}
}




////////////////////////////////////////////////////////////////////////////////////////////////////
//
// テープファイルをマウント
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void MountTapeFile(const TCHAR* strFile, int type)
{
	if (type == 1) {
		// CJR処理
		if (g_pTapeFormat != nullptr) {
			delete g_pTapeFormat;
			g_pTapeFormat = nullptr;
		}
		g_pTapeFormat = new CjrFormat();
		if (!g_pTapeFormat->Init(strFile))
			MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
	}
	else if (type == 2) {
		// JR2処理
		if (g_pTapeFormat != nullptr) {
			delete g_pTapeFormat;
			g_pTapeFormat = nullptr;
		}
		g_pTapeFormat = new Jr2Format();
		if (!g_pTapeFormat->Init(strFile))
			MessageBox(g_hMainWnd, g_strTable[(int)Msg::Failed_to_open_the_file], g_strTable[(int)Msg::Error], MB_OK | MB_ICONERROR);
	}

}


bool SetDebugLabel(const TCHAR* fileName)
{
	FILE* fp;
	long fileSize;
	char buf[1024];

	struct _stat stbuf;
	int st = _tstat(fileName, &stbuf);
	if (st != 0) return false;
	fileSize = stbuf.st_size;
	if (fileSize >= 1024 * 1024) {
		return false;
	}

	fp = _tfopen(fileName, _T("rt"));
	if (fp == nullptr) {
		return false;
	}

	g_debugLabel.clear();

	while (fgets(buf, 1023, fp)) {
		if (ferror(fp) != 0) {
			fclose(fp);
			return false;
		}

		std::string s(buf);
		std::wstring ws;

		try {
			ws = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(s);
		}
		catch (...){
			fclose(fp);
			return false;
		}

		std::wstringstream ss(ws);
		std::wstring s1, s2;

		ss >> s1;
		ss >> s2;

		TCHAR* e;
		unsigned int address = _tcstol(s1.c_str(), &e, 16);
		if (*e == '\0' && address > 0 && address < 66656) {
			wchar_t wc[DEBUGLABEL_NAME_LEN];
			wcsncpy(wc, s2.c_str(), DEBUGLABEL_NAME_LEN - 1);
			wc[DEBUGLABEL_NAME_LEN - 1] = L'\0';
			g_debugLabel.insert(std::make_pair(address, wc));
		}
	}
	fclose(fp);
	return true;

}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Trim関数（右側のみ）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int Rtrim(TCHAR *s) {
	int count = 0;

	if (s == NULL)
		return -1;

	int i = (int)_tcslen(s);

	while (--i >= 0 && s[i] == _T(' '))
		++count;

	s[i + 1] = _T('\0');

	return i + count;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバックウィンドウのウォッチ変数更新
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetWatchList()
{
	if (hWndDebug == NULL) return;

	HWND hWlist = GetDlgItem(hWndDebug, IDC_DEBUG_WLIST);

	for (unsigned int i = 0; i < watchVarList.size(); ++i) {
		WatchVar* w = watchVarList[i];
		TCHAR hex[10], dec[10];

		int val = sys.pAddress->ReadByteForDebug(w->address);
		if (w->size == 2) {
			val <<= 8;
			val += sys.pAddress->ReadByteForDebug(w->address + 1);
		}

		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT;
		wsprintf(hex, _T("%X"), val);
		lvItem.pszText = hex;
		lvItem.iItem = i;
		lvItem.iSubItem = 1;
		ListView_SetItem(hWlist, &lvItem);

		lvItem.mask = LVIF_TEXT;
		wsprintf(dec, _T("%d"), val);
		lvItem.pszText = dec;
		lvItem.iItem = i;
		lvItem.iSubItem = 2;
		ListView_SetItem(hWlist, &lvItem);

	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバックウィンドウのジャンプリスト更新
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void SetJumpList()
{
	if (hWndDebug == NULL) return;

	HWND hJumpList = GetDlgItem(hWndDebug, IDC_DEBUG_JUMPLIST);

	// ジャンプリスト
	SendMessage(hJumpList, WM_SETREDRAW, FALSE, 0);
	SendMessage(hJumpList, LB_RESETCONTENT, 0, 0);
	int idx = g_JumpHistory_index;
	if (--idx < 0)
		idx = JUMP_HISTORY_SIZE - 1;
	for (int i = 0; i < JUMP_HISTORY_SIZE; ++i) {
		SendMessage(hJumpList, LB_ADDSTRING, 0, (LPARAM)(g_JumpHistory[idx]));
		if (--idx < 0)
			idx = JUMP_HISTORY_SIZE - 1;
	}
	SendMessage(hJumpList, WM_SETREDRAW, TRUE, 0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 入力されたアドレスが正しいかどうかチェック
//
// 戻り値　正しければ int型のアドレス、不正なら -1、空欄は -2
//
////////////////////////////////////////////////////////////////////////////////////////////////////
int CheckAddress(TCHAR* s)
{
	Rtrim(s);
	if (_tcslen(s) == 0)
		return -2;
	
	TCHAR* e;
	unsigned int address = _tcstol(s, &e, 16);

	if (*e != '\0' || address < 0 || address > 65535) {
		return -1;
	}
	else {
		return address;
	}
}
