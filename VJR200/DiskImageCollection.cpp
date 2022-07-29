////////////////////////////////////////////////////////////
// DiskImage File Collection
//
// Written by Manuke

////////////////////////////////////////////////////////////
// include
#include "stdafx.h"
#include "StdHeader.h"
#include "DiskImageCollection.h"

////////////////////////////////////////////////////////////
// implementation of CDiskImageCollection

////////////////////////////////////////////////////////////
// create & destroy

// default constructor

CDiskImageCollection::CDiskImageCollection() :
	m_nFileIDSerial(0)
{
}

// destructor

CDiskImageCollection::~CDiskImageCollection() {
}

////////////////////////////////////////////////////////////
// add and delete

// add disk image file

bool CDiskImageCollection::AddDiskImageFile(
	const std::wstring& fstrFileName, bool bReadOnly)
{
	bool bResult = true;
	m_listDiskImageFile.push_back(CDiskImageFile(m_nFileIDSerial));
	iterator itImageFile = end();
	itImageFile--;
	int nResult = (*itImageFile).Load(fstrFileName, bReadOnly);
	if (nResult == CDiskImageFile::ERR_NOERROR) {
		m_nFileIDSerial++;
	} else {
		erase(itImageFile);
		bResult = false;
	}
	return bResult;
}

////////////////////////////////////////////////////////////
// operation

	// get disk image count

int CDiskImageCollection::GetDiskImageCount() {
	int nImageCount = 0;
	for (iterator itImageFile = begin(); itImageFile != end(); itImageFile++) {
		nImageCount += (*itImageFile).size();
	}
	return nImageCount;
}

// get disk image

CDiskImage* CDiskImageCollection::GetDiskImage(int nIndex) {
	CDiskImage* pDiskImage = NULL;
	for (iterator itImageFile = begin(); itImageFile != end(); itImageFile++) {
		if ((*itImageFile).size() > nIndex) {
			CDiskImageFile::iterator itImage = (*itImageFile).begin();
			std::advance(itImage, nIndex);
			pDiskImage = &(*itImage);
			break;
		} else {
			nIndex -= (*itImageFile).size();
		}
	}
	return pDiskImage;
}

// get index of disk image
//
//     pDiskImage
//         disk image
//     return value
//         index in image files
//         (-1 : not contained)

int CDiskImageCollection::GetDiskImageIndex(CDiskImage* pDiskImage) {
	if (pDiskImage == NULL) {
		return -1;
	}
	int nIndex = 0;
	iterator itImageFile;
	for (itImageFile = begin(); itImageFile != end(); itImageFile++) {
		if ((*itImageFile).GetFileID() == pDiskImage->GetFileID()) {
			int nIndex2 = (*itImageFile).GetDiskImageIndex(pDiskImage);
			if (nIndex2 < 0) {
				return -1;
			}
			nIndex += nIndex2;
			break;
		} else {
			nIndex += (*itImageFile).size();
		}
	}
	if (itImageFile == end()) {
		nIndex = -1;
	}
	return nIndex;
}
