////////////////////////////////////////////////////////////
// X88000 DiskImage Memory Manager
//
// Written by Manuke

////////////////////////////////////////////////////////////
// include
#include "stdafx.h"

#include "StdHeader.h"

#include "X88DiskImageMemory.h"

#include "DiskImageFile.h"

////////////////////////////////////////////////////////////
// implementation of CX88DiskImageMemory
// (disk image memory management class)

////////////////////////////////////////////////////////////
// create & destroy

// default constructor

CX88DiskImageMemory::CX88DiskImageMemory() :
	m_bReadOnly(true),
	m_pbtData(NULL),
	m_dwSize(0)
{
	m_hFile = NULL;
	m_hFileMap = NULL;
}

// standard constructor(buffer specified)

CX88DiskImageMemory::CX88DiskImageMemory(uint8_t* btData) :
	m_bReadOnly(true),
	m_pbtData(btData),
	m_dwSize(0)
{
	m_hFile = NULL;
	m_hFileMap = NULL;
}

// copy constructor(shallow copy)

CX88DiskImageMemory::CX88DiskImageMemory(
		const CX88DiskImageMemory& dimOther) :
	m_bReadOnly(dimOther.m_bReadOnly),
	m_pbtData(dimOther.m_pbtData),
	m_dwSize(dimOther.m_dwSize)
{
	m_hFile = dimOther.m_hFile;
	m_hFileMap = dimOther.m_hFileMap;
}

// destructor

CX88DiskImageMemory::~CX88DiskImageMemory() {
}

////////////////////////////////////////////////////////////
// operator

// let(shallow copy)

CX88DiskImageMemory& CX88DiskImageMemory::operator=(
	const CX88DiskImageMemory& dimOther)
{
	m_bReadOnly = dimOther.m_bReadOnly;
	m_pbtData = dimOther.m_pbtData;
	m_dwSize = dimOther.m_dwSize;

	m_hFile = dimOther.m_hFile;
	m_hFileMap = dimOther.m_hFileMap;

	return *this;
}

// compare(equal)

bool CX88DiskImageMemory::operator==(
	const CX88DiskImageMemory& dimOther) const
{
	return m_pbtData == dimOther.m_pbtData;
}

// compare(less)

bool CX88DiskImageMemory::operator<(
	const CX88DiskImageMemory& dimOther) const
{
	return m_pbtData < dimOther.m_pbtData;
}

////////////////////////////////////////////////////////////
// operation

// create

int CX88DiskImageMemory::Create(
	const std::wstring& fstrFileName, bool bReadOnly)
{
	m_bReadOnly = bReadOnly;
	m_pbtData = NULL;
	m_dwSize = 0;

	m_hFile = NULL;
	m_hFileMap = NULL;
	int nResult = CDiskImageFile::ERR_NOERROR;
	try {
		m_hFile = CreateFile(
			fstrFileName.c_str(),
			m_bReadOnly? GENERIC_READ: (GENERIC_READ | GENERIC_WRITE),
			m_bReadOnly? FILE_SHARE_READ: 0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (m_hFile == INVALID_HANDLE_VALUE) {
			if (m_bReadOnly) {
				m_hFile = NULL;
				throw int(CDiskImageFile::ERR_CANNOTOPEN);
			}
			m_bReadOnly = true;
			m_hFile = CreateFile(
				fstrFileName.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
			if (m_hFile == INVALID_HANDLE_VALUE) {
				m_hFile = NULL;
				throw int(CDiskImageFile::ERR_CANNOTOPEN);
			}
		}
		BY_HANDLE_FILE_INFORMATION hfi;
		if (!GetFileInformationByHandle(m_hFile, &hfi) ||
			(hfi.nFileSizeLow == 0))
		{
			throw int(CDiskImageFile::ERR_IOERROR);
		}
		m_dwSize = hfi.nFileSizeLow;
		if (!m_bReadOnly) {
			if ((m_hFileMap = CreateFileMapping(
					m_hFile,
					NULL, PAGE_READWRITE, 0, 0,
					NULL)) != NULL)
			{
				if ((m_pbtData = (uint8_t*)MapViewOfFile(
						m_hFileMap,
						FILE_MAP_WRITE, 0, 0, m_dwSize)) == NULL)
				{
					CloseHandle(m_hFileMap);
					m_hFileMap = NULL;
				}
			}
		}
		if (m_hFileMap == NULL) {
			if ((m_pbtData = (uint8_t*)malloc(m_dwSize)) == NULL) {
				throw int(CDiskImageFile::ERR_FEWMEMORY);
			}
			DWORD dwReadSize;
			if (!ReadFile(
					m_hFile, m_pbtData, m_dwSize, &dwReadSize, NULL))
			{
				throw int(CDiskImageFile::ERR_IOERROR);
			}
			if (m_bReadOnly) {
				CloseHandle(m_hFile);
				m_hFile = NULL;
			}
		}
	} catch (int nError) {
		Destroy();
		m_pbtData = NULL;
		m_dwSize = 0;
		m_hFile = NULL;
		m_hFileMap = NULL;
		nResult = nError;
	}

	return nResult;
}

// destroy

int CX88DiskImageMemory::Destroy() const {
	if (m_hFileMap != NULL) {
		if (m_pbtData != NULL) {
			UnmapViewOfFile(m_pbtData);
		}
	} else {
		if (m_pbtData != NULL) {
			free(m_pbtData);
		}
	}
	if (m_hFileMap != NULL) {
		CloseHandle(m_hFileMap);
	}
	if (m_hFile != NULL) {
		CloseHandle(m_hFile);
	}

	return CDiskImageFile::ERR_NOERROR;
}

// flush data

int CX88DiskImageMemory::Flush() const {
	int nResult = CDiskImageFile::ERR_NOERROR;

	if (m_pbtData != NULL) {
		if (m_hFileMap != NULL) {
		} else if (!m_bReadOnly && (m_hFile != NULL)) {
			DWORD dwWriteSize;
			if (!WriteFile(
					m_hFile, m_pbtData, m_dwSize, &dwWriteSize, NULL))
			{
				nResult = CDiskImageFile::ERR_IOERROR;
			}
		}
	}

	return nResult;
}
