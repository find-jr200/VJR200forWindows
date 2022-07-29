////////////////////////////////////////////////////////////
// DiskImage
//
// Written by Manuke

////////////////////////////////////////////////////////////
// include
#include "stdafx.h"
#include "StdHeader.h"

#include "DiskImage.h"
#include "vjr200.h"

////////////////////////////////////////////////////////////
// implementation of CDiskImage

////////////////////////////////////////////////////////////
// create & destroy

// default constructor

CDiskImage::CDiskImage() :
	m_nFileID(-1), m_nDiskID(-1),
	m_pbtDiskImage(NULL)
{
}

// constructor(file id specified)

CDiskImage::CDiskImage(int nFileID) :
	m_nFileID(nFileID), m_nDiskID(-1),
	m_pbtDiskImage(NULL)
{
}

// copy constructor

CDiskImage::CDiskImage(const CDiskImage& diOther) :
	m_nFileID(diOther.m_nFileID), m_nDiskID(diOther.m_nDiskID),
	m_pbtDiskImage(diOther.m_pbtDiskImage)
{
}

// destructor

CDiskImage::~CDiskImage() {
}

////////////////////////////////////////////////////////////
// operation

// set top pointer of disk image

uint32_t CDiskImage::SetDiskImagePtr(uint8_t* pbtDiskImage, uint32_t dwMaxSize) {
	m_pbtDiskImage = pbtDiskImage;
	if (m_pbtDiskImage == NULL) {
		return 0;
	}
	UGptr gptr = m_pbtDiskImage;
	UGptr gptr2 = gptr.m_pVoid;
	char ch;
	int nNameLength;
	for (nNameLength = 0; nNameLength < 17+9; nNameLength++) {
		gptr2 >> ch;
		if (ch == '\0') {
			break;
		}
	}
	if (nNameLength >= 17+9) {
		m_pbtDiskImage = NULL;
		return 0;
	}
	m_jstrImageName = StringToWString(string(gptr.m_pChar));
	gptr.m_pByte += 17+9;
	uint8_t bt;
	gptr >> bt;
	if ((bt != 0x00) && (bt != 0x10)) {
		m_pbtDiskImage = NULL;
		return 0;
	}
	m_bWriteProtected = (bt == 0x10);
	gptr >> bt;
	if ((bt != DISKTYPE_2D) &&
		(bt != DISKTYPE_2DD) &&
		(bt != DISKTYPE_2HD))
	{
		m_pbtDiskImage = NULL;
		return 0;
	}
	m_nDiskType = bt;
	gptr >> m_dwDiskSize;
	m_nTrackCount = TRACK_MAX;
	uint32_t dwMinSize = gptr.GetDiff(m_pbtDiskImage)+
		sizeof(uint32_t)*m_nTrackCount;
	if ((m_dwDiskSize > dwMaxSize) ||
		(m_dwDiskSize < dwMinSize))
	{
		m_pbtDiskImage = NULL;
		return 0;
	}
	int nPrevTrack = -1;
	for (int nTrack = 0; nTrack < TRACK_MAX; nTrack++) {
		if (nTrack < m_nTrackCount) {
			gptr >> m_adwTrackOffset[nTrack];
			if (m_adwTrackOffset[nTrack] > m_dwDiskSize) {
				m_adwTrackOffset[nTrack] = 0;
			}
		} else {
			m_adwTrackOffset[nTrack] = 0;
		}
		if (m_adwTrackOffset[nTrack] != 0) {
			if (m_adwTrackOffset[nTrack] < dwMinSize) {
				if ((nPrevTrack < 0) &&
					(m_adwTrackOffset[nTrack] == dwMinSize-sizeof(uint32_t)*4))
				{
					dwMinSize -= sizeof(uint32_t)*4;
					m_nTrackCount -= 4;
				} else {
					m_pbtDiskImage = NULL;
					return 0;
				}
			}
			if (nPrevTrack >= 0) {
				m_adwTrackSize[nPrevTrack] =
					m_adwTrackOffset[nTrack]-dwMinSize;
			}
			nPrevTrack = nTrack;
			dwMinSize = m_adwTrackOffset[nTrack];
		} else {
			m_adwTrackSize[nTrack] = 0;
		}
	}
	if (nPrevTrack >= 0) {
		m_adwTrackSize[nPrevTrack] = m_dwDiskSize-dwMinSize;
	}
	return m_dwDiskSize;
}

// set write protected

void CDiskImage::SetWriteProtect(bool bProtected) {
	if (m_pbtDiskImage != NULL) {
		m_bWriteProtected = bProtected;
		m_pbtDiskImage[17+9] = uint8_t(m_bWriteProtected? 0x10: 0x00);
	}
}
