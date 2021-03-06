﻿// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Aaron Giles
/***************************************************************************

    emucore.h

    General core utilities and macros used throughout the emulator.
***************************************************************************/
#pragma once

#ifndef MAME_EMU_EMUCORE_H
#define MAME_EMU_EMUCORE_H

#define LSB_FIRST

// standard C includes
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

// some cleanups for Solaris for things defined in stdlib.h
#if defined(__sun__) && defined(__svr4__)
#undef si_status
#undef WWORD
#endif

// standard C++ includes
#include <cassert>
#include <exception>
#include <type_traits>
#include <typeinfo>

// core system includes
//#include "osdcomm.h"
//#include "emualloc.h"
//#include "corestr.h"
//#include "bitmap.h"


//**************************************************************************
//  COMPILER-SPECIFIC NASTINESS
//**************************************************************************

// Suppress warnings about redefining the macro 'PPC' on LinuxPPC.
#undef PPC

// Suppress warnings about redefining the macro 'ARM' on ARM.
#undef ARM



//**************************************************************************
//  FUNDAMENTAL TYPES
//**************************************************************************

// genf is a generic function pointer; cast function pointers to this instead of void *
//typedef void genf(void);

// pen_t is used to represent pixel values in bitmaps
typedef uint32_t pen_t;

// stream_sample_t is used to represent a single sample in a sound stream
typedef int32_t stream_sample_t;

// running_machine is core to pretty much everything
//class running_machine;



//**************************************************************************
//  USEFUL COMPOSITE TYPES
//**************************************************************************

// generic_ptr is a union of pointers to various sizes
union generic_ptr
{
	generic_ptr(void *value) { v = value; }
	void *      v;
	int8_t *      i8;
	uint8_t *     u8;
	int16_t *     i16;
	uint16_t *    u16;
	int32_t *     i32;
	uint32_t *    u32;
	int64_t *     i64;
	uint64_t *    u64;
};


// PAIR is an endian-safe union useful for representing 32-bit CPU registers
union PAIR
{
#ifdef LSB_FIRST
	struct { uint8_t l,h,h2,h3; } b;
	struct { uint16_t l,h; } w;
	struct { int8_t l,h,h2,h3; } sb;
	struct { int16_t l,h; } sw;
#else
	struct { uint8_t h3,h2,h,l; } b;
	struct { int8_t h3,h2,h,l; } sb;
	struct { uint16_t h,l; } w;
	struct { int16_t h,l; } sw;
#endif
	uint32_t d;
	int32_t sd;
};


// PAIR16 is a 16-bit extension of a PAIR
union PAIR16
{
#ifdef LSB_FIRST
	struct { uint8_t l,h; } b;
	struct { int8_t l,h; } sb;
#else
	struct { uint8_t h,l; } b;
	struct { int8_t h,l; } sb;
#endif
	uint16_t w;
	int16_t sw;
};


// PAIR64 is a 64-bit extension of a PAIR
union PAIR64
{
#ifdef LSB_FIRST
	struct { uint8_t l,h,h2,h3,h4,h5,h6,h7; } b;
	struct { uint16_t l,h,h2,h3; } w;
	struct { uint32_t l,h; } d;
	struct { int8_t l,h,h2,h3,h4,h5,h6,h7; } sb;
	struct { int16_t l,h,h2,h3; } sw;
	struct { int32_t l,h; } sd;
#else
	struct { uint8_t h7,h6,h5,h4,h3,h2,h,l; } b;
	struct { uint16_t h3,h2,h,l; } w;
	struct { uint32_t h,l; } d;
	struct { int8_t h7,h6,h5,h4,h3,h2,h,l; } sb;
	struct { int16_t h3,h2,h,l; } sw;
	struct { int32_t h,l; } sd;
#endif
	uint64_t q;
	int64_t sq;
};



//**************************************************************************
//  COMMON CONSTANTS
//**************************************************************************

// constants for expression endianness
enum endianness_t
{
	ENDIANNESS_LITTLE,
	ENDIANNESS_BIG
};


// declare native endianness to be one or the other
#ifdef LSB_FIRST
const endianness_t ENDIANNESS_NATIVE = ENDIANNESS_LITTLE;
#else
const endianness_t ENDIANNESS_NATIVE = ENDIANNESS_BIG;
#endif


// M_PI is not part of the C/C++ standards and is not present on
// strict ANSI compilers or when compiling under GCC with -ansi
#ifndef M_PI
#define M_PI                            3.14159265358979323846
#endif


// orientation of bitmaps
#define ORIENTATION_FLIP_X              0x0001  /* mirror everything in the X direction */
#define ORIENTATION_FLIP_Y              0x0002  /* mirror everything in the Y direction */
#define ORIENTATION_SWAP_XY             0x0004  /* mirror along the top-left/bottom-right diagonal */

#define ROT0                            0
#define ROT90                           (ORIENTATION_SWAP_XY | ORIENTATION_FLIP_X)  /* rotate clockwise 90 degrees */
#define ROT180                          (ORIENTATION_FLIP_X | ORIENTATION_FLIP_Y)   /* rotate 180 degrees */
#define ROT270                          (ORIENTATION_SWAP_XY | ORIENTATION_FLIP_Y)  /* rotate dCounter-clockwise 90 degrees */



//**************************************************************************
//  COMMON MACROS
//**************************************************************************

// macro for defining a copy constructor and assignment operator to prevent copying
#define DISABLE_COPYING(TYPE) \
	TYPE(const TYPE &) = delete; \
	TYPE &operator=(const TYPE &) = delete

// macro for declaring enumerator operators that increment/decrement like plain old C
#define DECLARE_ENUM_OPERATORS(TYPE) \
inline TYPE &operator++(TYPE &value) { return value = TYPE(std::underlying_type_t<TYPE>(value) + 1); } \
inline TYPE operator++(TYPE &value, int) { TYPE const old(value); ++value; return old; } \
inline TYPE &operator--(TYPE &value) { return value = TYPE(std::underlying_type_t<TYPE>(value) - 1); } \
inline TYPE operator--(TYPE &value, int) { TYPE const old(value); --value; return old; }


// this macro passes an item followed by a string version of itself as two consecutive parameters
#define NAME(x) x, #x

// this macro wraps a function 'x' and can be used to pass a function followed by its name
#define FUNC(x) &x, #x


// standard assertion macros
#undef assert
#undef assert_always

#if defined(MAME_DEBUG_FAST)
#define assert(x)               do { } while (0)
#define assert_always(x, msg)   do { if (!(x)) throw emu_fatalerror("Fatal error: %s\nCaused by assert: %s:%d: %s", msg, __FILE__, __LINE__, #x); } while (0)
#elif defined(MAME_DEBUG)
#define assert(x)               do { if (!(x)) throw emu_fatalerror("assert: %s:%d: %s", __FILE__, __LINE__, #x); } while (0)
#define assert_always(x, msg)   do { if (!(x)) throw emu_fatalerror("Fatal error: %s\nCaused by assert: %s:%d: %s", msg, __FILE__, __LINE__, #x); } while (0)
#else
#define assert(x)               do { } while (0)
#define assert_always(x, msg)   do { if (!(x)) throw emu_fatalerror("Fatal error: %s (%s:%d)", msg, __FILE__, __LINE__); } while (0)
#endif


// macros to convert radians to degrees and degrees to radians
#define RADIAN_TO_DEGREE(x)   ((180.0 / M_PI) * (x))
#define DEGREE_TO_RADIAN(x)   ((M_PI / 180.0) * (x))


// endian-based value: first value is if 'endian' is little-endian, second is if 'endian' is big-endian
#define ENDIAN_VALUE_LE_BE(endian,leval,beval)  (((endian) == ENDIANNESS_LITTLE) ? (leval) : (beval))

// endian-based value: first value is if native endianness is little-endian, second is if native is big-endian
#define NATIVE_ENDIAN_VALUE_LE_BE(leval,beval)  ENDIAN_VALUE_LE_BE(ENDIANNESS_NATIVE, leval, beval)

// endian-based value: first value is if 'endian' matches native, second is if 'endian' doesn't match native
#define ENDIAN_VALUE_NE_NNE(endian,neval,nneval) (((endian) == ENDIANNESS_NATIVE) ? (neval) : (nneval))


// useful functions to deal with bit shuffling encryptions
template <typename T, typename U> constexpr T BIT(T x, U n) { return (x >> n) & T(1); }

template <typename T, typename U> constexpr T bitswap(T val, U b)
{
	return BIT(val, b) << 0U;
}

template <typename T, typename U, typename... V> constexpr T bitswap(T val, U b, V... c)
{
	return (BIT(val, b) << sizeof...(c)) | bitswap(val, c...);
}

// explicit versions that check number of bit position arguments
template <typename T, typename... U> constexpr T BITSWAP8(T val, U... b) { static_assert(sizeof...(b) == 8U, "wrong number of bits"); return bitswap(val, b...); }
template <typename T, typename... U> constexpr T BITSWAP16(T val, U... b) { static_assert(sizeof...(b) == 16U, "wrong number of bits"); return bitswap(val, b...); }
template <typename T, typename... U> constexpr T BITSWAP24(T val, U... b) { static_assert(sizeof...(b) == 24U, "wrong number of bits"); return bitswap(val, b...); }
template <typename T, typename... U> constexpr T BITSWAP32(T val, U... b) { static_assert(sizeof...(b) == 32U, "wrong number of bits"); return bitswap(val, b...); }
template <typename T, typename... U> constexpr T BITSWAP40(T val, U... b) { static_assert(sizeof...(b) == 40U, "wrong number of bits"); return bitswap(val, b...); }
template <typename T, typename... U> constexpr T BITSWAP48(T val, U... b) { static_assert(sizeof...(b) == 48U, "wrong number of bits"); return bitswap(val, b...); }
template <typename T, typename... U> constexpr T BITSWAP56(T val, U... b) { static_assert(sizeof...(b) == 56U, "wrong number of bits"); return bitswap(val, b...); }
template <typename T, typename... U> constexpr T BITSWAP64(T val, U... b) { static_assert(sizeof...(b) == 64U, "wrong number of bits"); return bitswap(val, b...); }



////**************************************************************************
////  EXCEPTION CLASSES
////**************************************************************************
//
//// emu_exception is the base class for all emu-related exceptions
//class emu_exception : public std::exception { };
//
//
//// emu_fatalerror is a generic fatal exception that provides an error string
//class emu_fatalerror : public emu_exception
//{
//public:
//	emu_fatalerror(const char *format, ...) ATTR_PRINTF(2,3);
//	emu_fatalerror(const char *format, va_list ap);
//	emu_fatalerror(int _exitcode, const char *format, ...) ATTR_PRINTF(3,4);
//	emu_fatalerror(int _exitcode, const char *format, va_list ap);
//
//	const char *string() const { return text; }
//	int exitcode() const { return code; }
//
//private:
//	char text[1024];
//	int code;
//};
//
//class tag_add_exception
//{
//public:
//	tag_add_exception(const char *tag) : m_tag(tag) { }
//	const char *tag() const { return m_tag.c_str(); }
//private:
//	std::string m_tag;
//};

//**************************************************************************
//  CASTING TEMPLATES
//**************************************************************************

class device_t;

void report_bad_cast(const std::type_info &src_type, const std::type_info &dst_type);
void report_bad_device_cast(const device_t *dev, const std::type_info &src_type, const std::type_info &dst_type);

template <typename Dest, typename Source>
inline std::enable_if_t<std::is_base_of<device_t, Source>::value> report_bad_cast(Source *const src)
{
	if (src) report_bad_device_cast(src, typeid(Source), typeid(Dest));
	else report_bad_cast(typeid(Source), typeid(Dest));
}

template <typename Dest, typename Source>
inline std::enable_if_t<!std::is_base_of<device_t, Source>::value> report_bad_cast(Source *const src)
{
	device_t const *dev(dynamic_cast<device_t const *>(src));
	if (dev) report_bad_device_cast(dev, typeid(Source), typeid(Dest));
	else report_bad_cast(typeid(Source), typeid(Dest));
}

// template function for casting from a base class to a derived class that is checked
// in debug builds and fast in release builds
template <typename Dest, typename Source>
inline Dest downcast(Source *src)
{
#if defined(MAME_DEBUG) && !defined(MAME_DEBUG_FAST)
	Dest const chk(dynamic_cast<Dest>(src));
	if (chk != src) report_bad_cast<std::remove_pointer_t<Dest>, Source>(src);
#endif
	return static_cast<Dest>(src);
}

template<class Dest, class Source>
inline Dest downcast(Source &src)
{
#if defined(MAME_DEBUG) && !defined(MAME_DEBUG_FAST)
	std::remove_reference_t<Dest> *const chk(dynamic_cast<std::remove_reference_t<Dest> *>(&src));
	if (chk != &src) report_bad_cast<std::remove_reference_t<Dest>, Source>(&src);
#endif
	return static_cast<Dest>(src);
}

// template function which takes a strongly typed enumerator and returns its value as a compile-time constant
template <typename E>
using enable_enum_t = typename std::enable_if_t<std::is_enum<E>::value, typename std::underlying_type_t<E>>;

template <typename E>
constexpr inline enable_enum_t<E>
underlying_value(E e) noexcept
{
	return static_cast< typename std::underlying_type<E>::type >( e );
}

// template function which takes an integral value and returns its representation as enumerator (even strongly typed)
template <typename E , typename T>
constexpr inline typename std::enable_if_t<std::is_enum<E>::value && std::is_integral<T>::value, E>
enum_value(T value) noexcept
{
	return static_cast<E>(value);
}



////**************************************************************************
////  FUNCTION PROTOTYPES
////**************************************************************************
//
//[[noreturn]] void fatalerror(const char *format, ...) ATTR_PRINTF(1,2);
//[[noreturn]] void fatalerror_exitcode(running_machine &machine, int exitcode, const char *format, ...) ATTR_PRINTF(3,4);
//
////**************************************************************************
////  INLINE FUNCTIONS
////**************************************************************************
//
//// population count
//#if !defined(__NetBSD__)
//inline int popcount(uint32_t val)
//{
//	int count;
//
//	for (count = 0; val != 0; count++)
//		val &= val - 1;
//	return count;
//}
//#endif


// convert a series of 32 bits into a float
inline float u2f(uint32_t v)
{
	union {
		float ff;
		uint32_t vv;
	} u;
	u.vv = v;
	return u.ff;
}


// convert a float into a series of 32 bits
inline uint32_t f2u(float f)
{
	union {
		float ff;
		uint32_t vv;
	} u;
	u.ff = f;
	return u.vv;
}


// convert a series of 64 bits into a double
inline double u2d(uint64_t v)
{
	union {
		double dd;
		uint64_t vv;
	} u;
	u.vv = v;
	return u.dd;
}


// convert a double into a series of 64 bits
inline uint64_t d2u(double d)
{
	union {
		double dd;
		uint64_t vv;
	} u;
	u.dd = d;
	return u.vv;
}

#endif  /* MAME_EMU_EMUCORE_H */
