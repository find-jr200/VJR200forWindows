////////////////////////////////////////////////////////////
// DiskImage File Collection
//
// Written by Manuke

#ifndef DiskImageCollection_DEFINED
#define DiskImageCollection_DEFINED

////////////////////////////////////////////////////////////
// declare

class CDiskImageCollection;

////////////////////////////////////////////////////////////
// include

#include "DiskImageFile.h"

////////////////////////////////////////////////////////////
// declaration of CDiskImageCollection

class CDiskImageCollection {
// attribute
protected:
	// file id serial
	int m_nFileIDSerial;
	// list of image files
	std::list<CDiskImageFile> m_listDiskImageFile;

// STL
public:
	typedef std::list<CDiskImageFile>::iterator iterator;

	void clear() {
		m_listDiskImageFile.clear();
	}
	iterator begin() {
		return m_listDiskImageFile.begin();
	}
	iterator end() {
		return m_listDiskImageFile.end();
	}
	int size() {
		return (int)m_listDiskImageFile.size();
	}
	iterator erase(iterator itImageFile) {
		return m_listDiskImageFile.erase(itImageFile);
	}

// create & destroy
public:
	// default constructor
	CDiskImageCollection();
	// destructor
	~CDiskImageCollection();

// add and delete
public:
	// add disk image file
	bool AddDiskImageFile(
		const std::wstring& fstrFileName, bool bReadOnly);

// operation
public:
	// get disk image count
	int GetDiskImageCount();
	// get disk image
	CDiskImage* GetDiskImage(int nIndex);
	// get index of disk image
	int GetDiskImageIndex(CDiskImage* pDiskImage);
};

#endif // DiskImageCollection_DEFINED
