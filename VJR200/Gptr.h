////////////////////////////////////////////////////////////
// General Pointer
//
// Written by Manuke

#ifndef Gptr_DEFINED
#define Gptr_DEFINED

#include <cstdint>

////////////////////////////////////////////////////////////
// declaration and implementation of UGptr

union UGptr {
// attribute
public:
	// pointers
	void* m_pVoid;
	char* m_pChar;
	uint8_t* m_pByte;
	uint16_t* m_pWord;
	uint32_t* m_pDword;

// create & destroy
public:
	// default constructor
	UGptr() {
		m_pVoid = NULL;
	}
	// constructor(pointer specified)
	UGptr(void* pVoid) {
		m_pVoid = pVoid;
	}

// operator
public:
	// let
	UGptr& operator=(void* pVoid) {
		m_pVoid = pVoid;
		return *this;
	}
	// right shift operator(read char)
	UGptr& operator>>(char& ch) {
		ch = *(m_pChar++);
		return *this;
	}
	// left shift operator(write char)
	UGptr& operator<<(char ch) {
		*(m_pChar++) = ch;
		return *this;
	}
	// right shift operator(read uint8_t)
	UGptr& operator>>(uint8_t& bt) {
		bt = *(m_pByte++);
		return *this;
	}
	// left shift operator(write uint8_t)
	UGptr& operator<<(uint8_t bt) {
		*(m_pByte++) = bt;
		return *this;
	}
	// right shift operator(read uint16_t)
	UGptr& operator>>(uint16_t& w) {

#ifdef X88_BYTEORDER_LITTLE_ENDIAN

		w = *(m_pWord++);

#else // X88_BYTEORDER_BIG_ENDIAN

		w = (uint16_t)*(m_pByte++);
		w |= (uint16_t)(*(m_pByte++) << 8);

#endif // X88_BYTEORDER

		return *this;
	}
	// left shift operator(write uint16_t)
	UGptr& operator<<(uint16_t w) {

#ifdef X88_BYTEORDER_LITTLE_ENDIAN

		*(m_pWord++) = w;

#else // X88_BYTEORDER_BIG_ENDIAN

		*(m_pByte++) = (uint8_t)w;
		*(m_pByte++) = (uint8_t)(w >> 8);

#endif // X88_BYTEORDER

		return *this;
	}
	// right shift operator(read uint32_t)
	UGptr& operator>>(uint32_t& dw) {

#ifdef X88_BYTEORDER_LITTLE_ENDIAN

		dw = *(m_pDword++);

#else // X88_BYTEORDER_BIG_ENDIAN

		dw = (uint32_t)*(m_pByte++);
		dw |= (uint32_t)(*(m_pByte++) << 8);
		dw |= (uint32_t)(*(m_pByte++) << 16);
		dw |= (uint32_t)(*(m_pByte++) << 24);

#endif // X88_BYTEORDER

		return *this;
	}
	// left shift operator(write uint32_t)
	UGptr& operator<<(uint32_t dw) {

#ifdef X88_BYTEORDER_LITTLE_ENDIAN

		*(m_pDword++) = dw;

#else // X88_BYTEORDER_BIG_ENDIAN

		*(m_pByte++) = (uint8_t)dw;
		*(m_pByte++) = (uint8_t)(dw >> 8);
		*(m_pByte++) = (uint8_t)(dw >> 16);
		*(m_pByte++) = (uint8_t)(dw >> 24);

#endif // X88_BYTEORDER

		return *this;
	}

// operation
public:
	// read sequential data
	void Get(char* pch, int nSize) {
		memcpy(pch, m_pChar, nSize);
		m_pChar += nSize;
	}
	// write sequential data
	void Set(char* pch, int nSize) {
		memcpy(m_pChar, pch, nSize);
		m_pChar += nSize;
	}
	// get difference
	int GetDiff(void* pvStart) {
		return (int)(m_pChar-(char*)pvStart);
	}
};

#endif // Gptr_DEFINED
