// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __VJR200_H__
#define __VJR200_H__
#include <deque>
#include <map>
#include "resource.h"
#include "MemWindow.h"
#include "DisasmWindow.h"
#include "ITapeFormat.h"

using namespace std;

// 定数
extern const int CLOCK;
extern const int BITMAP_W;
extern const int BITMAP_H;
extern const float REAL_WRATIO;
extern const int STR_RESOURCE_NUM;
extern const unsigned int RECENT_FILES_NUM;
extern const int STATUSBAR_HEIGHT;
extern const int CJR_FILE_MAX;
extern const int CPU_SPEED_MAX;
extern const int BREAKPOINT_NUM;
#define SOUNDBUFFER_BLOCK_NUM 5
extern const TCHAR* APPDATA_PATH;
extern const uint8_t g_gcharCode1[][14];
extern const uint8_t g_gcharCode2[][14];
extern const int CEREAL_VER;

// グローバル変数:
#ifndef _ANDROID
extern HINSTANCE g_hInst;
extern HWND g_hMainWnd;
extern HWND g_hMemWnd;
extern HMODULE g_hMod;
extern ID2D1Factory* g_pD2dFactory;
extern IDWriteFactory* g_pDWriteFactory;
extern MemWindow* g_pMemWindow;
extern DisasmWindow* g_pDisasmWindow;
extern HANDLE g_hEvent[][6];
extern BYTE g_bits1[448], g_bits2[336];
#endif
extern int g_dramWait;
extern bool g_deviceRunning;
extern int g_debug;
extern int g_breakPoint[];
extern bool g_bSoundOn;
extern bool g_bMemWindow;
extern bool g_bDisasmWindow;
extern TCHAR g_strTable[][256];
extern bool g_bForcedJoystick;
extern int g_vcyncCounter;
extern ITapeFormat* g_pTapeFormat;


// ダイアログ・ini保存項目
extern int g_cpuScale;
extern int g_viewScale;
extern bool g_bSmoothing;
extern bool g_bFpsCpu;
extern int g_soundVolume;
extern bool g_bRamExpand1;
extern bool g_bRamExpand2;
extern int g_ramInitPattern;
#ifndef _ANDROID
extern RECT g_windowPos;
#endif
extern TCHAR g_pRomFile[];
extern TCHAR g_pFontFile[];
extern deque<wstring> g_rFilesforCJRSet;
extern deque<wstring> g_rFilesforQLoad;
extern deque<wstring> g_macroStrings;
extern int g_refRate;
extern bool g_bRefRateAuto;
extern bool g_bSquarePixel;
extern bool g_bOverClockLoad;
extern int g_quickTypeS;
extern int g_language;
extern int g_keyboard;
extern int g_stereo1, g_stereo2, g_stereo3;
extern bool g_bBGPlay;
extern int g_prOutput;
extern int g_prMaker;
extern int g_prAutoFeed;
extern bool g_b2buttons;
extern int g_Joypad1pA, g_Joypad1pB, g_Joypad2pA, g_Joypad2pB;
extern int g_forcedJoypadA, g_forcedJoypadB; 
extern bool g_bForcedJoystick;
extern int g_sBufferWriteInterval;
extern int g_bCmtAddBlank;
extern bool g_bRomajiKana;
extern bool g_bPrinterLed;
extern bool g_bStatusBar;
extern std::map<int, std::wstring> g_debugLabel;


ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL OpenFileDialog(TCHAR * filter, TCHAR * fileName, HWND hwnd);
INT_PTR CALLBACK DlgSaveCjrProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ChangeWindowSize(HMENU hMenu);
void SetMenuItemState(HMENU hMenu);
int GetRefreshRate();
INT_PTR CALLBACK DlgMacroProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
INT_PTR CALLBACK DlgVGraphKeybProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
INT_PTR CALLBACK DlgOptionProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgDebugProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
INT_PTR CALLBACK DlgRomFontProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID CALLBACK FpsTimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired);
bool CreateSoundThreads();
unsigned __stdcall SoundSetThread0(void *p);
unsigned __stdcall SoundSetThread1(void *p);
unsigned __stdcall SoundSetThread2(void *p);
void DeleteSoundThreads();
void AddRecentFiles(const TCHAR* str, int mode);
void GetStateFilePathName(TCHAR pathName[], int idx);
void SetMenuItemForRecentFiles();
void SetMenuItemForStateSaveLoad();
int CheckFileFormat(const TCHAR* fileName);
void MountTapeFile(const TCHAR* strFile, int type);
bool SetDebugLabel(const TCHAR* fileName);
int Rtrim(TCHAR *s);
void SetWatchList();

enum class Msg {
	It_will_not_work_on_this_version_of_Windows = 1,
	Failed_to_create_application_settings,
	Initialization_failed,
	Creation_of_D2D_device_failed,
	CJR_File,
	Text_File,
	Do_you_want_to_reset_it,
	Confirmation,
	ROM_file_is_missing_or_incorrect,
	FONT_file_is_missing_or_incorrect, // 10
	Refresh_rate_incorrect,
	Please_specify_the_file,
	Caution,
	The_specified_ROM_file_can_not_be_opened,
	The_size_of_the_specified_ROM_file_is_incorrect, // 15
	The_specified_FONT_file_can_not_be_opened,
	The_size_of_the_specified_FONT_file_is_incorrect,
	The_specified_file_can_not_be_opened,
	FONT_File,
	Please_specify_the_file_name_for_JR, // 20
	The_file_name_for_JR_is_limited_to_16_characters,
	There_is_no_program,
	Failed_to_open_the_file,
	Start_address_is_incorrect,
	Address_range_is_from_0_to_FFFF, // 25
	End_address_is_incorrect,
	Invalid_range_specification,
	Memory_Window,
	Keyboard_acquisition_error,
	Emulator, // 30
	System,
	Sound,
	Printer,
	Joypad,
	Press_the_joypad_button, // 35
	Press_the_key_to_assign,
	Invalid_file_format,
	Disassemble_Window,
	This_file_cant_be_processed,
	Help_Url
};


#ifndef _ANDROID
template<class Interface>
inline void SafeRelease(
	Interface **ppInterfaceToRelease
)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}
#endif

#endif


