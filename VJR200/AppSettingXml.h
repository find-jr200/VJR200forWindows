// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __APPSETTINGXML_H__
#define __APPSETTINGXML_H__
#include "tinyxml2.h"

class AppSettingXml
{
public:
	AppSettingXml();
	bool Init();
	bool Read();
	void Write();
	~AppSettingXml();
protected:
	TCHAR fileName[MAX_PATH];
	template<class T> void SetNumValue(tinyxml2::XMLElement * parent, char * name, T& var);
	void SetBoolValue(tinyxml2::XMLElement* parent, char* name, bool& var);
	void SetStrValue(tinyxml2::XMLElement* parent, char* name, TCHAR* var);
};

#endif