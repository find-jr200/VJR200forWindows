// license:BSD-3-Clause
// copyright-holders:FIND
#ifndef __FDDSYSTEM_H__
#define __FDDSYSTEM_H__
#include <cstdint>
#include <set>

#include "PC88Fdc.h"
#include "DiskImageCollection.h"
#include "X88DiskImageMemory.h"
#include <cereal/cereal.hpp>

class FddSystem
{
public:
	FddSystem();
	virtual ~FddSystem();

	void TickCounter(int cycles);
	uint8_t Read(int r);
	uint8_t ReadForDebug(int r);
	void Write(int r, uint8_t b);
	void Reset();

	// open disk image file
	static int DiskImageFileOpen(
		const std::wstring& fstrFileName, bool& bReadOnly,
		uint8_t*& pbtData, uint32_t& dwSize);
	// close disk image file
	static int DiskImageFileClose(uint8_t* pbtData);
	// FDD ICON
	void FDDActive(int driveNum);
	void CountUp();

	bool GetFddStatus(int driveNo);

	CPC88Fdc* pFdc;
	static CDiskImageCollection& GetDiskImageCollection();

	std::wstring mountedFileName[2];

	template <class Archive>
	void serialize(Archive & ar, std::uint32_t const version);

protected:
	static const float LEDON_TIME;

	// disk image memory manager
	static std::set<CX88DiskImageMemory> m_setX88DiskImageMemory;
	static CDiskImageCollection d88files;
	static void IntVectChangeSub();

	uint8_t reg10;

	float count1 = 0, count2 = 0;
	bool bCount1On = false, bCount2On = false;

};


template<class Archive>
inline void FddSystem::serialize(Archive & ar, std::uint32_t const version)
{
	ar(reg10);
	ar(count1, count2);
	ar(bCount1On, bCount2On);
};

#endif