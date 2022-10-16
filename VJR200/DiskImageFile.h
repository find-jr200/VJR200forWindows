////////////////////////////////////////////////////////////
// DiskImage File
//
// Written by Manuke

#ifndef DiskImageFile_DEFINED
#define DiskImageFile_DEFINED

////////////////////////////////////////////////////////////
// declare

class CDiskImageFile;

////////////////////////////////////////////////////////////
// include
#include "DiskImage.h"

////////////////////////////////////////////////////////////
// declaration of CDiskImageFile

class CDiskImageFile {
// typedef
public:
	// type of file open callback function
	typedef int (*DiskImageFileOpenCallback)(
		const std::wstring& fstrFileName, bool& bReadOnly,
		uint8_t*& pbtData, uint32_t& dwSize);
	// type of file close callback function
	typedef int (*DiskImageFileCloseCallback)(uint8_t* pbtData);

// enum
public:
	// error code
	enum {
		ERR_NOERROR,
		ERR_ERROR,
		ERR_CANNOTOPEN,
		ERR_IOERROR,
		ERR_FEWMEMORY,
		ERR_BADSTRUCTURE
	};

// attribute
protected:
	// file id
	int m_nFileID;
	// disk id serial
	int m_nDiskIDSerial;
	// file name(filesystem encoding)
	std::wstring m_fstrFileName;
	// read-only
	bool m_bReadOnly;
	// data buffer
	uint8_t* m_pbtData;
	// data size
	uint32_t m_dwSize;
	// list of disk images
	std::list<CDiskImage> m_listDiskImage;

	// file open callback function
	static DiskImageFileOpenCallback m_pDiskImageFileOpenCallback;
	// file close callback function
	static DiskImageFileCloseCallback m_pDiskImageFileCloseCallback;

public:
	// get file id
	int GetFileID() const {
		return m_nFileID;
	}
	// set file id
	void SetFileID(int nFileID) {
		m_nFileID = nFileID;
	}
	// get file name(filesystem encoding)
	std::wstring GetFileName() const {
		return m_fstrFileName;
	}
	// is read-only
	bool IsReadOnly() const {
		return m_bReadOnly;
	}
	// set file open callback function
	static void SetDiskImageFileOpenCallback(
		DiskImageFileOpenCallback pDiskImageFileOpenCallback)
	{
		m_pDiskImageFileOpenCallback = pDiskImageFileOpenCallback;
	}
	// set file close callback function
	static void SetDiskImageFileCloseCallback(
		DiskImageFileCloseCallback pDiskImageFileCloseCallback)
	{
		m_pDiskImageFileCloseCallback = pDiskImageFileCloseCallback;
	}

// STL
public:
	typedef std::list<CDiskImage>::iterator iterator;

	void clear() {
		m_fstrFileName = _T("");
		m_bReadOnly = false;
		if (m_pbtData != NULL) {
			m_pDiskImageFileCloseCallback(m_pbtData);
			m_pbtData = NULL;
			m_dwSize = 0;
		}
		m_listDiskImage.clear();
	}
	iterator begin() {
		return m_listDiskImage.begin();
	}
	iterator end() {
		return m_listDiskImage.end();
	}
	int size() {
		return (int)m_listDiskImage.size();
	}
	iterator erase(iterator itImage) {
		return m_listDiskImage.erase(itImage);
	}

// create & destroy
public:
	// default constructor
	CDiskImageFile();
	// constructor(file id specified)
	CDiskImageFile(int nFileID);
	// copy constructor
	CDiskImageFile(const CDiskImageFile& difOther);
	// destructor
	~CDiskImageFile();

	// load disk image
	int Load(const std::wstring& fstrFileName, bool bReadOnly);

// operation
public:
	// get index of disk image
	int GetDiskImageIndex(CDiskImage* pDiskImage);
};

#endif // DiskImageFile_DEFINED
