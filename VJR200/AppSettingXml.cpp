// license:BSD-3-Clause
// copyright-holders:FIND
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// class AppSettingXml
// 設定保存 XMLファイルの読み出し、書き込みを担当
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AppSettingXml.h"
#include <Shlobj.h>
#include <Shlwapi.h>
#include <string>
#include <codecvt>
#include <unordered_map> 
#include "tinyxml2.h"
#include "VJR200.h"

using namespace std;

AppSettingXml::AppSettingXml()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 初期化
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AppSettingXml::Init()
{
	SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, fileName);
	wcscat(fileName, L"\\FIND_JR");
	if (!PathFileExists(fileName))
		CreateDirectory(fileName, 0);
	wcscat(fileName, L"\\VJR200");
	if (!PathFileExists(fileName))
		CreateDirectory(fileName, 0);
	wcscat(fileName, L"\\vjr200.xml");

	if (!PathFileExists(fileName))
		return false;
	else
		return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 読み込んだ要素の値を変数に設定するサポート関数（int, long専用）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T> void AppSettingXml::SetNumValue(tinyxml2::XMLElement* parent, char* name, T& var)
{
	const char* c;
	if (parent != nullptr) {
		tinyxml2::XMLElement* e = parent->FirstChildElement(name);
		if (e != nullptr) {
			c = e->GetText();
			if (c != nullptr && strcmp(c, "")) var = atoi(c);
		}
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 読み込んだ要素の値を変数に設定するサポート関数（bool専用）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void AppSettingXml::SetBoolValue(tinyxml2::XMLElement* parent, char* name, bool& var)
{
	const char* c;
	if (parent != nullptr) {
		tinyxml2::XMLElement* e = parent->FirstChildElement(name);
		if (e != nullptr) {
			c = e->GetText();
			if (c != nullptr && strcmp(c, "")) var = strcmp(c, "0") ? true : false;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 読み込んだ要素の値を変数に設定するサポート関数（TCHAR*専用）
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void AppSettingXml::SetStrValue(tinyxml2::XMLElement* parent, char* name, TCHAR* var)
{
	wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	const char* c;

	if (parent != nullptr) {
		tinyxml2::XMLElement* e = parent->FirstChildElement(name);
		if (e != nullptr) {
			c = e->GetText();
			if (c != nullptr && strcmp(c, "")) _tcscpy(var, converter.from_bytes(c).c_str());
		}
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XMLファイル読み込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AppSettingXml::Read()
{
	TCHAR buff[MAX_PATH] = {};

	tinyxml2::XMLDocument doc;
	FILE* fp = _tfopen(fileName, _T("rb"));
	if (fp == nullptr)
		return false;
	if (doc.LoadFile(fp) != tinyxml2::XML_SUCCESS) {
		fclose(fp);
		return false;
	}
	fclose(fp);

	tinyxml2::XMLElement* root = doc.FirstChildElement();
	const char* c;
	// Window ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* window = root->FirstChildElement("Window");
	if (window != nullptr) {
		SetNumValue<long>(window, "PosX", g_windowPos.left);
		SetNumValue<long>(window, "PosY", g_windowPos.top);
		SetNumValue<int>(window, "ViewScale", g_viewScale);
		SetBoolValue(window, "SquarePixel", g_bSquarePixel);
		SetBoolValue(window, "Smoothing", g_bSmoothing);
		SetBoolValue(window, "PrinterLed", g_bPrinterLed);
		SetBoolValue(window, "StatusBar", g_bStatusBar);
	}

	// PATH ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* path = root->FirstChildElement("Path");
	wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	char elemName[30];
	if (path != nullptr) {
		SetStrValue(path, "RomFile", g_pRomFile);
		SetStrValue(path, "FontFile", g_pFontFile);

		for (unsigned int i = 0; i < RECENT_FILES_NUM; ++i) {
			sprintf(elemName, "%s%d", "RFilesforCJRSet", i);
			tinyxml2::XMLElement* e;
			if ((e = path->FirstChildElement(elemName)) != nullptr) {
				c = e->GetText();
				if (c != nullptr && strcmp(c, ""))
					g_rFilesforCJRSet.push_back(converter.from_bytes(c).c_str());
				else
					g_rFilesforCJRSet.push_back(_T(""));
			}
			else {
				g_rFilesforCJRSet.push_back(_T(""));
			}
		}

		for (unsigned int i = 0; i < RECENT_FILES_NUM; ++i) {
			sprintf(elemName, "%s%d", "RFilesforQLoad", i);
			tinyxml2::XMLElement* e;
			if ((e = path->FirstChildElement(elemName)) != nullptr) {
				c = e->GetText();
				if (c != nullptr && strcmp(c, ""))
					g_rFilesforQLoad.push_back(converter.from_bytes(c).c_str());
				else
					g_rFilesforQLoad.push_back(_T(""));
			}
			else {
				g_rFilesforQLoad.push_back(_T(""));
			}
		}
	}

	// MACRO //////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* macro = root->FirstChildElement("Macro");
	if (macro != nullptr) {
		for (unsigned int i = 0; i < 10; ++i) {
			sprintf(elemName, "%s%d", "Macro", i);
			tinyxml2::XMLElement* e;
			if ((e = macro->FirstChildElement(elemName)) != nullptr) {
				c = e->GetText();
				if (c != nullptr && strcmp(c, ""))
					g_macroStrings.push_back(converter.from_bytes(c).c_str());
				// else
					// g_macroStrings.push_back(_T(""));
			}
			else {
				// g_macroStrings.push_back(_T(""));
			}
		}
	}


	// Options ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* optioins = root->FirstChildElement("Options");
	if (optioins != nullptr) {
		SetBoolValue(optioins, "FpsCpu", g_bFpsCpu);
		SetBoolValue(optioins, "RamExpand1", g_bRamExpand1);
		SetBoolValue(optioins, "RamExpand2", g_bRamExpand2);
		SetNumValue<int>(optioins, "RamInitPattern", g_ramInitPattern);
		SetNumValue<int>(optioins, "RefRate", g_refRate);
		SetBoolValue(optioins, "RefRateAuto", g_bRefRateAuto);
		SetNumValue<int>(optioins, "SoundVolume", g_soundVolume);
		SetBoolValue(optioins, "OverClockLoad", g_bOverClockLoad);
		SetNumValue<int>(optioins, "CmtAddBlank", g_bCmtAddBlank);
		SetNumValue<int>(optioins, "QuickType", g_quickTypeS);
		SetNumValue<int>(optioins, "Language", g_language);
		SetNumValue<int>(optioins, "Keyboard", g_keyboard);
		SetBoolValue(optioins, "RomajiKana", g_bRomajiKana);
	}


	// Sound ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* sound = root->FirstChildElement("Sound");
	if (sound != nullptr) {
		SetNumValue<int>(sound, "Stereo1", g_stereo1);
		SetNumValue<int>(sound, "Stereo2", g_stereo2);
		SetNumValue<int>(sound, "Stereo3", g_stereo3);
		SetBoolValue(sound, "BgPlay", g_bBGPlay);
		SetNumValue<int>(sound, "SBufferWriteInterval", g_sBufferWriteInterval);
	}


	// Printer ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* printer = root->FirstChildElement("Printer");
	if (printer != nullptr) {
		SetNumValue<int>(printer, "Output", g_prOutput);
		SetNumValue<int>(printer, "Maker", g_prMaker);
		SetNumValue<int>(printer, "AutoFeed", g_prAutoFeed);
	}


	// Joypad ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* joypad = root->FirstChildElement("Joypad");
	if (joypad != nullptr) {
		SetBoolValue(joypad, "TwoButtons", g_b2buttons);
		SetNumValue<int>(joypad, "Button1pA", g_Joypad1pA);
		SetNumValue<int>(joypad, "Button1pB", g_Joypad1pB);
		SetNumValue<int>(joypad, "Button2pA", g_Joypad2pA);
		SetNumValue<int>(joypad, "Button2pB", g_Joypad2pB);
		SetNumValue<int>(joypad, "ForcedJoystickA", g_forcedJoypadA);
		SetNumValue<int>(joypad, "ForcedJoystickB", g_forcedJoypadB);
	}

	return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XMLファイル書き込み
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void AppSettingXml::Write()
{

	char buff[MAX_PATH];

	wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	tinyxml2::XMLDocument xml;
	tinyxml2::XMLElement* root = xml.NewElement("root");
	xml.InsertFirstChild(root);

	// Window ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* window = root->GetDocument()->NewElement("Window");
	root->InsertEndChild(window);
	tinyxml2::XMLElement* elem;

	elem = xml.NewElement("PosX");
	elem->SetText(g_windowPos.left);
	window->InsertEndChild(elem);

	elem = xml.NewElement("PosY");
	elem->SetText(g_windowPos.top);
	window->InsertEndChild(elem);

	elem = xml.NewElement("ViewScale");
	elem->SetText(g_viewScale);
	window->InsertEndChild(elem);

	sprintf(buff, "%d", g_bSquarePixel ? 1 : 0);
	elem = xml.NewElement("SquarePixel");
	elem->SetText(buff);
	window->InsertEndChild(elem);

	sprintf(buff, "%d", g_bSmoothing ? 1 : 0);
	elem = xml.NewElement("Smoothing");
	elem->SetText(buff);
	window->InsertEndChild(elem);

	sprintf(buff, "%d", g_bPrinterLed ? 1 : 0);
	elem = xml.NewElement("PrinterLed");
	elem->SetText(buff);
	window->InsertEndChild(elem);

	sprintf(buff, "%d", g_bStatusBar ? 1 : 0);
	elem = xml.NewElement("StatusBar");
	elem->SetText(buff);
	window->InsertEndChild(elem);

	// PATH ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* path = root->GetDocument()->NewElement("Path");
	root->InsertEndChild(path);

	elem = xml.NewElement("RomFile");
	elem->SetText(converter.to_bytes(g_pRomFile).c_str());
	path->InsertEndChild(elem);

	elem = xml.NewElement("FontFile");
	elem->SetText(converter.to_bytes(g_pFontFile).c_str());
	path->InsertEndChild(elem);

	char elemName[MAX_PATH];
	elemName[0] = '\0';
	if (g_rFilesforCJRSet.size() >= RECENT_FILES_NUM + 1)
		while (g_rFilesforCJRSet.size() > RECENT_FILES_NUM) g_rFilesforCJRSet.pop_back();
	for (unsigned int i = 0; i < g_rFilesforCJRSet.size(); ++i) {
		sprintf(elemName, "%s%d", "RFilesforCJRSet", i);
		elem = xml.NewElement(elemName);
		elem->SetText(converter.to_bytes(g_rFilesforCJRSet[i]).c_str());
		path->InsertEndChild(elem);
	}

	if (g_rFilesforQLoad.size() >= RECENT_FILES_NUM + 1)
		while (g_rFilesforQLoad.size() > RECENT_FILES_NUM) g_rFilesforQLoad.pop_back();
	for (unsigned int i = 0; i < g_rFilesforQLoad.size(); ++i) {
		sprintf(elemName, "%s%d", "RFilesforQLoad", i);
		elem = xml.NewElement(elemName);
		elem->SetText(converter.to_bytes(g_rFilesforQLoad[i]).c_str());
		path->InsertEndChild(elem);
	}

	// Macro ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* macro = root->GetDocument()->NewElement("Macro");
	root->InsertEndChild(macro);

	if (g_macroStrings.size() > 10)
		while (g_macroStrings.size() > 10) g_macroStrings.pop_back();
	for (unsigned int i = 0; i < g_macroStrings.size(); ++i) {
		sprintf(elemName, "%s%d", "Macro", i);
		elem = xml.NewElement(elemName);
		elem->SetText(converter.to_bytes(g_macroStrings[i]).c_str());
		macro->InsertEndChild(elem);
	}

	// Options ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* options = root->GetDocument()->NewElement("Options");
	root->InsertEndChild(options);

	sprintf(buff, "%d", g_bFpsCpu ? 1 : 0);
	elem = xml.NewElement("FpsCpu");
	elem->SetText(buff);
	options->InsertEndChild(elem);

	sprintf(buff, "%d", g_bRamExpand1 ? 1 : 0);
	elem = xml.NewElement("RamExpand1");
	elem->SetText(buff);
	options->InsertEndChild(elem);

	sprintf(buff, "%d", g_bRamExpand2 ? 1 : 0);
	elem = xml.NewElement("RamExpand2");
	elem->SetText(buff);
	options->InsertEndChild(elem);

	elem = xml.NewElement("RamInitPattern");
	elem->SetText(g_ramInitPattern);
	options->InsertEndChild(elem);

	elem = xml.NewElement("RefRate");
	elem->SetText(g_refRate);
	options->InsertEndChild(elem);

	sprintf(buff, "%d", g_bRefRateAuto ? 1 : 0);
	elem = xml.NewElement("RefRateAuto");
	elem->SetText(buff);
	options->InsertEndChild(elem);

	elem = xml.NewElement("SoundVolume");
	elem->SetText(g_soundVolume);
	options->InsertEndChild(elem);

	sprintf(buff, "%d", g_bOverClockLoad ? 1 : 0);
	elem = xml.NewElement("OverClockLoad");
	elem->SetText(buff);
	options->InsertEndChild(elem);

	sprintf(buff, "%d", g_bCmtAddBlank ? 1 : 0);
	elem = xml.NewElement("CmtAddBlank");
	elem->SetText(buff);
	options->InsertEndChild(elem);

	elem = xml.NewElement("QuickType");
	elem->SetText(g_quickTypeS);
	options->InsertEndChild(elem);

	elem = xml.NewElement("Language");
	elem->SetText(g_language);
	options->InsertEndChild(elem);

	elem = xml.NewElement("Keyboard");
	elem->SetText(g_keyboard);
	options->InsertEndChild(elem);

	sprintf(buff, "%d", g_bRomajiKana ? 1 : 0);
	elem = xml.NewElement("RomajiKana");
	elem->SetText(buff);
	options->InsertEndChild(elem);

	// Sound ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* sound = root->GetDocument()->NewElement("Sound");
	root->InsertEndChild(sound);

	elem = xml.NewElement("Stereo1");
	elem->SetText(g_stereo1);
	sound->InsertEndChild(elem);

	elem = xml.NewElement("Stereo2");
	elem->SetText(g_stereo2);
	sound->InsertEndChild(elem);

	elem = xml.NewElement("Stereo3");
	elem->SetText(g_stereo3);
	sound->InsertEndChild(elem);

	sprintf(buff, "%d", g_bBGPlay ? 1 : 0);
	elem = xml.NewElement("BgPlay");
	elem->SetText(buff);
	sound->InsertEndChild(elem);

	elem = xml.NewElement("SBufferWriteInterval");
	elem->SetText(g_sBufferWriteInterval);
	sound->InsertEndChild(elem);


	// Printer ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* printer = root->GetDocument()->NewElement("Printer");
	root->InsertEndChild(printer);

	elem = xml.NewElement("Output");
	elem->SetText(g_prOutput);
	printer->InsertEndChild(elem);

	elem = xml.NewElement("Maker");
	elem->SetText(g_prMaker);
	printer->InsertEndChild(elem);

	elem = xml.NewElement("AutoFeed");
	elem->SetText(g_prAutoFeed);
	printer->InsertEndChild(elem);

	// Joypad ///////////////////////////////////////////////////////////////////////
	tinyxml2::XMLElement* joypad = root->GetDocument()->NewElement("Joypad");
	root->InsertEndChild(joypad);

	sprintf(buff, "%d", g_b2buttons ? 1 : 0);
	elem = xml.NewElement("TwoButtons");
	elem->SetText(buff);
	joypad->InsertEndChild(elem);

	elem = xml.NewElement("Button1pA");
	elem->SetText(g_Joypad1pA);
	joypad->InsertEndChild(elem);

	elem = xml.NewElement("Button1pB");
	elem->SetText(g_Joypad1pB);
	joypad->InsertEndChild(elem);

	elem = xml.NewElement("Button2pA");
	elem->SetText(g_Joypad2pA);
	joypad->InsertEndChild(elem);

	elem = xml.NewElement("Button2pB");
	elem->SetText(g_Joypad2pB);
	joypad->InsertEndChild(elem);

	elem = xml.NewElement("ForcedJoystickA");
	elem->SetText(g_forcedJoypadA);
	joypad->InsertEndChild(elem);

	elem = xml.NewElement("ForcedJoystickB");
	elem->SetText(g_forcedJoypadB);
	joypad->InsertEndChild(elem);

	SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, fileName);
	wcscat(fileName, L"\\FIND_JR");
	if (!PathFileExists(fileName))
		CreateDirectory(fileName, 0);
	wcscat(fileName, L"\\VJR200");
	if (!PathFileExists(fileName))
		CreateDirectory(fileName, 0);
	wcscat(fileName, L"\\vjr200.xml");

	FILE* fp = _tfopen(fileName, _T("wb"));
	fprintf(fp, "<?xml version = \"1.0\" encoding = \"utf-8\"?>\r\n");
	xml.SaveFile(fp);
	fclose(fp);
}



AppSettingXml::~AppSettingXml()
{
}
