// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __APPSETTING_H__
#define __APPSETTING_H__

class AppSetting
{
public:
	AppSetting();
	bool Init();
	bool Read();
	void Write();
	~AppSetting();
protected:
	TCHAR fileName[MAX_PATH];
};

#endif