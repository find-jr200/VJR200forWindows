// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class DisasmWindow
// 逆アセンブルリストを表示するクラス
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <vector>
#include "VJR200.h"
#include "JRSystem.h"
#include "DisasmWindow.h"

static const LPCWSTR szWindowClassMem = L"Disassemble Window";
static const int OPERAND_MAX = 40;
static const int LINE_MAX = 60;
static int lineHeight = 16;
static int dispno, pos;

 const wchar_t DisasmWindow::msc_fontName[] = L"Courier New";
const FLOAT DisasmWindow::msc_fontSize = 14;

const wchar_t *const DisasmWindow::nimonic[256] = {
	L"NA",L"NOP",L"NA",L"NA",L"NA",L"NA",L"TAP",L"TPA",L"INX",L"DEX",L"CLV",L"SEV",L"CLC",L"SEC",L"CLI",L"SEI",
	L"SBA",L"CBA",L"NA",L"NA",L"NA",L"NA",L"TAB",L"TBA",L"NA(DAA)",L"DAA",L"NA(ABA)",L"ABA",L"NA",L"NA",L"NA",L"NA",
	L"BRA",L"NA",L"BHI",L"BLS",L"BCC",L"BCS",L"BNE",L"BEQ",L"BVC",L"BVS",L"BPL",L"BMI",L"BGE",L"BLT",L"BGT",L"BLE",
	L"TSX",L"INS",L"PULA",L"PULB",L"DES",L"TXS",L"PSHA",L"PSHB",L"NA",L"RTS",L"NA(RTI)",L"RTI",L"NA",L"NA",L"WAI",L"SWI",
	L"NEGA",L"NA",L"NA(COMA)",L"COMA",L"LSRA",L"NA",L"RORA",L"ASRA",L"ASLA",L"ROLA",L"DECA",L"NA(DECA)",L"INCA",L"TSTA",L"NA",L"CLRA",
	L"NEGB",L"NA",L"NA(COMB)",L"COMB",L"LSRB",L"NA",L"RORB",L"ASRB",L"ASLB",L"ROLB",L"DECB",L"NA(DECB)",L"INCB",L"TSTB",L"NA",L"CLRB",
	L"NEG",L"NA(NEG)",L"NA(COM)",L"COM",L"LSR",L"NA(LSR)",L"ROR",L"ASR",L"ASL",L"ROL",L"DEC",L"NA(DEC)",L"INC",L"TST",L"JMP",L"CLR",
	L"NEG",L"NA(NEG)",L"NA(COM)",L"COM",L"LSR",L"NA(LSR)",L"ROR",L"ASR",L"ASL",L"ROL",L"DEC",L"NA(DEC)",L"INC",L"TST",L"JMP",L"CLR",
	L"SUBA",L"CMPA",L"SBCA",L"NA(SBCA)",L"ANDA",L"BITA",L"LDAA",L"NA(STAA)",L"EORA",L"ADCA",L"ORAA",L"ADDA",L"CPX",L"BSR",L"LDS",L"NA(STS)",
	L"SUBA",L"CMPA",L"SBCA",L"NA(SBCA)",L"ANDA",L"BITA",L"LDAA",L"STAA",L"EORA",L"ADCA",L"ORAA",L"ADDA",L"CPX",L"NA",L"LDS",L"STS",
	L"SUBA",L"CMPA",L"SBCA",L"NA(SBCA)",L"ANDA",L"BITA",L"LDAA",L"STAA",L"EORA",L"ADCA",L"ORAA",L"ADDA",L"CPX",L"JSR",L"LDS",L"STS",
	L"SUBA",L"CMPA",L"SBCA",L"NA(SBCA)",L"ANDA",L"BITA",L"LDAA",L"STAA",L"EORA",L"ADCA",L"ORAA",L"ADDA",L"CPX",L"JSR",L"LDS",L"STS",
	L"SUBB",L"CMPB",L"SBCB",L"NA(SBCB)",L"ANDB",L"BITB",L"LDAB",L"NA(STAB)",L"EORB",L"ADCB",L"ORAB",L"ADDB",L"NA",L"NA",L"LDX",L"NA(STS)",
	L"SUBB",L"CMPB",L"SBCB",L"NA(SBCB)",L"ANDB",L"BITB",L"LDAB",L"STAB",L"EORB",L"ADCB",L"ORAB",L"ADDB",L"NA",L"NA",L"LDX",L"STX",
	L"SUBB",L"CMPB",L"SBCB",L"NA(SBCB)",L"ANDB",L"BITB",L"LDAB",L"STAB",L"EORB",L"ADCB",L"ORAB",L"ADDB",L"NA",L"NA(JSR)",L"LDX",L"STX",
	L"SUBB",L"CMPB",L"SBCB",L"NA(SBCB)",L"ANDB",L"BITB",L"LDAB",L"STAB",L"EORB",L"ADCB",L"ORAB",L"ADDB",L"NA",L"NA(JSR)",L"LDX",L"STX" };

const int DisasmWindow::bytes[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	2,2,2,2,2,2,2,2,2,2,2,2,3,2,3,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	2,2,2,2,2,2,2,2,2,2,2,2,1,1,3,2,
	2,2,2,2,2,2,2,2,2,2,2,2,1,1,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,1,3,3,3 };

const int DisasmWindow::addressing[256] = {
	0,4,0,0,0,0,4,4,4,4,4,4,4,4,4,4,
	4,4,0,0,0,0,4,4,4,4,4,4,0,0,0,0,
	5,0,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	4,4,4,4,4,4,4,4,0,4,4,4,0,0,4,4,
	4,0,4,4,4,0,4,4,4,4,4,4,4,4,0,4,
	4,0,4,4,4,0,4,4,4,4,4,4,4,4,0,4,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,0,3,3,3 };


enum class AddressingMode { IMMEDIATE = 0, DIRECT = 1, INDEX = 2, EXTEND = 3, IMPLIED = 4, RELATIVEAD = 5 };

extern JRSystem sys;

DisasmWindow::DisasmWindow(): hwnd(NULL),
pRenderTarget(NULL),
pTextFormat(NULL),
pBlackBrush(NULL),
pTextLayout(NULL)
{
}

DisasmWindow::~DisasmWindow()
{
	SafeRelease(&pTextFormat);
	SafeRelease(&pTextLayout);

	DiscardDeviceResources();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT DisasmWindow::Init()
{
	HRESULT hr;
	hr = CreateDeviceIndependentResources();

	if (SUCCEEDED(hr)) {
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wcex.lpfnWndProc = WndProcDisasmWindow;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = g_hInst;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = szWindowClassMem;
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		RegisterClassEx(&wcex);

		RECT rect;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		hwnd = CreateWindow(
			szWindowClassMem,
			g_strTable[(int)Msg::Disassemble_Window],
			WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
			rect.right - 300,
			0,
			300,
			rect.bottom,
			g_hMainWnd,
			NULL,
			g_hInst,
			nullptr
		);

		HMENU hMenu = LoadMenu(g_hMod, MAKEINTRESOURCE(IDC_DISASM));
		SetMenu(hwnd, hMenu);

		HACCEL hAccelTable = LoadAccelerators(g_hInst, MAKEINTRESOURCE(IDC_DISASM));

		hr = hwnd ? S_OK : E_FAIL;
		if (SUCCEEDED(hr))
		{
			ShowWindow(hwnd, SW_SHOWNA);
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウハンドルを返す
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HWND DisasmWindow::GetHWnd()
{
	return hwnd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウ表示
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void DisasmWindow::Show()
{
	ShowWindow(hwnd, SW_SHOWNA);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバイス独立リソース生成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT DisasmWindow::CreateDeviceIndependentResources()
{
	HRESULT hr;

	// Create a DirectWrite text format object.
	hr = g_pDWriteFactory->CreateTextFormat(
		msc_fontName,
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		msc_fontSize,
		L"", //locale
		&pTextFormat
	);

	if (SUCCEEDED(hr))
	{
		// Center the text horizontally and vertically.
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバイスリソース生成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT DisasmWindow::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if (!pRenderTarget)
	{

		RECT rc;
		GetClientRect(hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
		);

		// Create a Direct2D render target.
		hr = g_pD2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, size),
			&pRenderTarget
		);
		if (SUCCEEDED(hr))
		{
			// Create a black brush.
			hr = pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				&pBlackBrush
			);
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DirecWrite描画
//
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT DisasmWindow::OnRender()
{
	HRESULT hr;

	hr = CreateDeviceResources();

	if (SUCCEEDED(hr) && !(pRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		// Retrieve the size of the render target.
		D2D1_SIZE_F renderTargetSize = pRenderTarget->GetSize();

		pRenderTarget->BeginDraw();
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		wchar_t  text[8000];
		D2D1_POINT_2F points;
		points.x = 10.0f;
		points.y = 0.0f;
		text[0] = '\0';

		uint16_t pc, sp, x;
		uint8_t a, b, cc;
		sys.pCpu->GetRegister(pc, sp, x, a, b, cc);
		curAddress = pc;
		for (int i = 0; i <  dispno; ++i) {
			OperationSet oSet;
			wchar_t line[LINE_MAX];
			prevAddress = curAddress;
			SetOperation(&oSet);
			if (curAddress - prevAddress < 0) break;
			wsprintf(line, L"%04X %s%s %s\r\n", oSet.startAddress, oSet.bytesString, oSet.nimonic, oSet.operand);


			auto ws = g_debugLabel[oSet.startAddress];
			if (ws != L"") {
				++i;
				_TCHAR str[LINE_MAX] ;
				wsprintf(str, L"%s:--------\r\n", ws.c_str());
				wcscat(text, str);
			}

			wcscat(text, line);
		}

		hr = g_pDWriteFactory->CreateTextLayout(
			text,
			(UINT32)wcslen(text),
			pTextFormat,
			renderTargetSize.width,
			renderTargetSize.height,
			&pTextLayout
		);

		if (SUCCEEDED(hr))
			pRenderTarget->DrawTextLayout(points, pTextLayout, pBlackBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
		SafeRelease(&pTextLayout);

		hr = pRenderTarget->EndDraw();

		if (hr == D2DERR_RECREATE_TARGET)
		{
			hr = S_OK;
			DiscardDeviceResources();
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// リサイズ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void DisasmWindow::OnResize(UINT width, UINT height)
{
	if (pRenderTarget)
	{
		D2D1_SIZE_U size;
		size.width = width;
		size.height = height;

		pRenderTarget->Resize(size);
	}

}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウを閉じる
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void DisasmWindow::Close()
{
	SendMessage(hwnd, WM_CLOSE, 0, 0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// デバイスリソース廃棄
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void DisasmWindow::DiscardDeviceResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBlackBrush);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1命令文の情報セット
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void DisasmWindow::SetOperation(OperationSet* oSet)
{
	int bytesCount;
	int opCode;
	vector<uint8_t> byteList;
	wchar_t bytesString[30] = {};
	
	opCode = sys.pAddress->ReadByte(curAddress);
	bytesCount = bytes[opCode];
	oSet->startAddress = curAddress;

	for (int i = 0; i < bytesCount; ++i)
	{
		wchar_t buf[20] = {};
		wsprintf(buf, L"%02X ", (int)sys.pAddress->ReadByte(curAddress));
		wcscat(bytesString, buf);
		byteList.push_back(sys.pAddress->ReadByte(curAddress));
		++curAddress;
	}
	wcscat(bytesString, L"          ");
	wcsncpy(oSet->bytesString, bytesString, 10);
	oSet->bytesString[10] = 0;
	for (unsigned int i = 0; i < byteList.size(); ++i) {
		oSet->byteArray[i] = byteList[i];
	}
	wcscpy(oSet->nimonic, nimonic[opCode]);
	wchar_t str[OPERAND_MAX] = {};
	MakeOperand(str, byteList, addressing[opCode], bytes[opCode]);
	wcscpy(oSet->operand, str);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// オペランド作成
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void DisasmWindow::MakeOperand(wchar_t* s, vector<uint8_t>& ops, int amode, int bytes)
{
	wchar_t w[OPERAND_MAX] = {};

	switch (amode)
	{
	case (int)AddressingMode::IMMEDIATE:
		if (bytes == 1)
			wcscpy(s, L"");
		else if (bytes == 2) {
			// wchar_t w[10] = {};
			wsprintf(w, L"#%02X", ops[1]);
			wcscpy(s, w);
		}
		else {
			wsprintf(w, L"#%02X%02X", ops[1], ops[2]);
			wcscpy(s, w);
		}
		break;
	case (int)AddressingMode::DIRECT:
	{
		auto label = g_debugLabel[(int)ops[1]];
		if (label == L"")
			wsprintf(w, L"%02X", ops[1]);
		else
			wsprintf(w, L"%s", label.c_str());
			
		wcscpy(s, w);
		break;
	}
	case (int)AddressingMode::INDEX:
	{
		wsprintf(w, L"%02X,X", ops[1]);
		wcscpy(s, w);
		break;
	}
	case (int)AddressingMode::EXTEND:
	{
		int address = ops[1];
		address <<= 8;
		address += ops[2];
		auto label = g_debugLabel[address];
		if (label == L"")
			wsprintf(w, L"%02X%02X", ops[1], ops[2]);
		else
			wsprintf(w, L"%s", label.c_str());

		wcscpy(s, w);
		break;
	}
	case (int)AddressingMode::RELATIVEAD:
	{

		int i = (int8_t)(ops[1]) + curAddress;
		wsprintf(w, L"%02X", (uint16_t)i);
		wcscpy(s, w);
		break;
	}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ウィンドウプロシージャ
//
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProcDisasmWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	bool wasHandled = false;
	RECT rect;

	switch (message) {
	case WM_SIZE:
	{
		UINT w = LOWORD(lParam);
		UINT h = HIWORD(lParam);
		g_pDisasmWindow->OnResize(w, h);

		GetClientRect(hWnd, &rect);
		dispno = rect.bottom / lineHeight;
		break;
	}
	case WM_PAINT:
	case WM_DISPLAYCHANGE:
	{
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		g_pDisasmWindow->OnRender();
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		g_bDisasmWindow = false;
		CheckMenuItem(GetMenu(g_hMainWnd), IDM_TOOL_DISASSEMBLEWINDOW, MF_UNCHECKED);
		delete g_pDisasmWindow;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


