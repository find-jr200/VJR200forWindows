////////////////////////////////////////////////////////////
// X88000 DiskImage Memory Manager
//
// Written by Manuke

#ifndef X88DiskImageMemory_DEFINED
#define X88DiskImageMemory_DEFINED

////////////////////////////////////////////////////////////
// declare

class CX88DiskImageMemory;

////////////////////////////////////////////////////////////
// declaration of CX88DiskImageMemory
// (disk image memory management class)

class CX88DiskImageMemory {
// attribute
protected:
	// read-only attribute
	bool m_bReadOnly;
	// memory for disk image
	uint8_t* m_pbtData;
	// byte size of disk image
	uint32_t m_dwSize;

//#ifdef X88_PLATFORM_WINDOWS

	// file handle
	HANDLE m_hFile;
	// memory-mapped file handle
	HANDLE m_hFileMap;

//#elif defined(X88_PLATFORM_UNIX)
//
//	// file descriptor
//	int m_nFile;
//	// memory-mapped file
//	bool m_bMemoryMappedFile;
//
//#endif // X88_PLATFORM

public:
	// is read-only  attribute
	bool IsReadOnly() const {
		return m_bReadOnly;
	}
	// get memory for disk image
	uint8_t* GetData() const {
		return m_pbtData;
	}
	// get byte size of disk image
	uint32_t GetSize() const {
		return m_dwSize;
	}

// create & destroy
public:
	// default constructor
	CX88DiskImageMemory();
	// standard constructor(buffer specified)
	CX88DiskImageMemory(uint8_t* btData);
	// copy constructor(shallow copy)
	CX88DiskImageMemory(const CX88DiskImageMemory& dimOther);
	// destructor
	~CX88DiskImageMemory();

// operator
public:
	// let(shallow copy)
	CX88DiskImageMemory& operator=(
		const CX88DiskImageMemory& dimOther);
	// compare(equal)
	bool operator==(const CX88DiskImageMemory& dimOther) const;
	// compare(less)
	bool operator<(const CX88DiskImageMemory& dimOther) const;

// operation
public:
	// create
	int Create(
		const std::wstring& fstrFileName, bool bReadOnly);
	// destroy
	int Destroy() const;
	// flush data
	int Flush() const;
};

#endif // X88DiskImageMemory_DEFINED
