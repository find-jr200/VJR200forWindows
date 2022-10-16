////////////////////////////////////////////////////////////
// DiskImage
//
// Written by Manuke

#ifndef DiskImage_DEFINED
#define DiskImage_DEFINED

////////////////////////////////////////////////////////////
// declare

class CDiskImage;

////////////////////////////////////////////////////////////
// include

#include "Gptr.h"
#include <string>

////////////////////////////////////////////////////////////
// declaration of CDiskImage

class CDiskImage {
// enum
public:
	// disk type
	enum {
		DISKTYPE_2D  = 0x00,
		DISKTYPE_2DD = 0x10,
		DISKTYPE_2HD = 0x20
	};
	// max track count
	enum {
		TRACK_MAX = 164
	};

// attribute
protected:
	// file id
	int m_nFileID;
	// disk id
	int m_nDiskID;
	// disk image
	uint8_t* m_pbtDiskImage;
	// image name(utf16 encoding)
	std::wstring m_jstrImageName;
	// write protected flag
	bool m_bWriteProtected;
	// disk type
	int m_nDiskType;
	// track count
	int m_nTrackCount;
	// disk size
	uint32_t m_dwDiskSize;
	// track offset
	uint32_t m_adwTrackOffset[TRACK_MAX];
	// track size
	uint32_t m_adwTrackSize[TRACK_MAX];

public:
	// get file id
	int GetFileID() const {
		return m_nFileID;
	}
	// get disk id
	int GetDiskID() const {
		return m_nDiskID;
	}
	// set disk id
	void SetDiskID(int nDiskID) {
		m_nDiskID = nDiskID;
	}
	// get top pointer of disk image
	uint8_t* GetDiskImagePtr() {
		return m_pbtDiskImage;
	}
	// get image name(SJIS encoding)
	std::wstring GetImageName() const {
		return m_jstrImageName;
	}
	// is write protected
	bool IsWriteProtected() const {
		return m_bWriteProtected;
	}
	// get disk type
	int GetDiskType() const {
		return m_nDiskType;
	}
	// get disk size
	uint32_t GetDiskSize() const {
		return m_dwDiskSize;
	}
	// offset top pointer of disk image
	void OffsetDiskImagePtr(int nOffset) {
		if (m_pbtDiskImage != NULL) {
			m_pbtDiskImage += nOffset;
		}
	}
	// get top pointer of track image
	uint8_t* GetTrackImagePtr(int nTrack) {
		return m_pbtDiskImage+m_adwTrackOffset[nTrack];
	}
	// get track size
	uint32_t GetTrackSize(int nTrack) {
		return m_adwTrackSize[nTrack];
	}

// create & destroy
public:
	// default constructor
	CDiskImage();
	// constructor(file id specified)
	CDiskImage(int nFileID);
	// copy constructor
	CDiskImage(const CDiskImage& diOther);
	// destructor
	~CDiskImage();

// operator
public:
	// compare(equal)
	bool operator==(const CDiskImage& diOther) const {
		return (m_nFileID == diOther.m_nFileID) &&
			(m_nDiskID == diOther.m_nDiskID);
	}

// operation
public:
	// set top pointer of disk image
	uint32_t SetDiskImagePtr(uint8_t* pbtDiskImage, uint32_t dwMaxSize);
	// set write protected
	void SetWriteProtect(bool bProtected);
};

#endif // DiskImage_DEFINED
