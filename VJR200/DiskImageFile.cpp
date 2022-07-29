////////////////////////////////////////////////////////////
// DiskImage File
//
// Written by Manuke

////////////////////////////////////////////////////////////
// include
#include "stdafx.h"
#include "StdHeader.h"

#include "DiskImageFile.h"

////////////////////////////////////////////////////////////
// implementation of CDiskImageFile

////////////////////////////////////////////////////////////
// attribute

// file open callback function

CDiskImageFile::DiskImageFileOpenCallback
	CDiskImageFile::m_pDiskImageFileOpenCallback = NULL;

// file close callback function

CDiskImageFile::DiskImageFileCloseCallback
	CDiskImageFile::m_pDiskImageFileCloseCallback = NULL;

////////////////////////////////////////////////////////////
// create & destroy

// default constructor

CDiskImageFile::CDiskImageFile() :
	m_nFileID(-1), m_nDiskIDSerial(0),
	m_bReadOnly(false),
	m_pbtData(NULL), m_dwSize(0)
{
}

// constructor(file id specified)

CDiskImageFile::CDiskImageFile(int nFileID) :
	m_nFileID(nFileID), m_nDiskIDSerial(0),
	m_bReadOnly(false),
	m_pbtData(NULL), m_dwSize(0)
{
}

// copy constructor

CDiskImageFile::CDiskImageFile(const CDiskImageFile& difOther) :
	m_nFileID(difOther.m_nFileID),
	m_nDiskIDSerial(difOther.m_nDiskIDSerial),
	m_fstrFileName(difOther.m_fstrFileName),
	m_bReadOnly(difOther.m_bReadOnly),
	m_pbtData(difOther.m_pbtData), m_dwSize(difOther.m_dwSize),
	m_listDiskImage(difOther.m_listDiskImage)
{
}

// destructor

CDiskImageFile::~CDiskImageFile() {
	clear();
}

// load disk image

int CDiskImageFile::Load(
	const std::wstring& fstrFileName, bool bReadOnly)
{
	m_bReadOnly = bReadOnly;
	// m_pDiskImageFileOpenCallback‚ÅŒÄ‚Î‚ê‚éŠÖ”‚ÍFddSystem::DiskImageFileOpen
	int nResult = m_pDiskImageFileOpenCallback(fstrFileName, m_bReadOnly, m_pbtData, m_dwSize);
		if (nResult == ERR_NOERROR) {
		uint8_t* pbtData = m_pbtData;
		uint32_t dwLeft = m_dwSize;
		do {
			m_listDiskImage.push_back(m_nFileID);
			iterator itImage = end();
			itImage--;
			uint32_t dwImageSize = (*itImage).SetDiskImagePtr(pbtData, dwLeft);
			if (dwImageSize > 0) {
				(*itImage).SetDiskID(m_nDiskIDSerial++);
				dwLeft -= dwImageSize;
				pbtData += dwImageSize;
			} else {
				erase(itImage);
				dwLeft = 0;
			}
		} while (dwLeft > 0);
		if (size() <= 0) {
			clear();
			nResult = ERR_BADSTRUCTURE;
		} else {
			m_fstrFileName = fstrFileName;
		}
	}
	return nResult;
}

////////////////////////////////////////////////////////////
// operation

// get index of disk image
//
//     pDiskImage
//         disk image
//     retrun
//         index in images
//         (-1 : not contained)

int CDiskImageFile::GetDiskImageIndex(CDiskImage* pDiskImage) {
	if (GetFileID() != pDiskImage->GetFileID()) {
		return -1;
	}
	int nIndex = 0;
	iterator itImage;
	for (itImage = begin(); itImage != end(); itImage++) {
		if (((*itImage).GetFileID() == pDiskImage->GetFileID()) &&
			((*itImage).GetDiskID() == pDiskImage->GetDiskID()))
		{
			break;
		} else {
			nIndex++;
		}
	}
	if (itImage == end()) {
		nIndex = -1;
	}
	return nIndex;
}
