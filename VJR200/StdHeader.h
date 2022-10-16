////////////////////////////////////////////////////////////
// standard header for pre-compile
//
// Written by Manuke

////////////////////////////////////////////////////////////
// platform & compiler

#ifdef _MSC_VER // Windows platform & Visual C++

// conditional expression is constant
#pragma warning(disable : 4127)
// nonstandard extension used(in windows.h)
#pragma warning(disable : 4201)
// private operator= are good to have
#pragma warning(disable : 4512)
// cannot create copy constructor
#pragma warning(disable : 4511)
// delete unused inline function
#pragma warning(disable : 4514)
// unreachable code?(Unknown)
#pragma warning(disable : 4702)
// unreferenced inline/local function has been removed
#pragma warning(disable : 4710)
// identifier was truncated in the debug information
#pragma warning(disable : 4786)
// old-type function
#pragma warning(disable : 4996)

#ifdef WIN_GTK // WinGTK+ API

// unreferenced local variable has been removed
#pragma warning(disable : 4505)

#endif // WIN_GTK

#define X88_COMPILER_TEMPLATE_EXPLICIT_SUPPORT
#define X88_COMPILER_TEMPLATE_STATICVAL_NORMAL

#elif defined(__BORLANDC__) // Windows platform & Borland C++

// unreferenced inline/local function has been removed
#pragma warning(disable : 4710)

#define X88_COMPILER_TEMPLATE_EXPLICIT_SUPPORT
#define X88_COMPILER_TEMPLATE_STATICVAL_NORMAL

#else // g++

#define X88_COMPILER_TEMPLATE_EXPLICIT_SUPPORT
#define X88_COMPILER_TEMPLATE_STATICVAL_SPECIAL

#endif // compiler check

#ifdef _WINDOWS // Windows Platform

#ifndef WIN_GTK // Windows API

#define X88_PLATFORM_WINDOWS
#define X88_GUI_WINDOWS
#define X88_ENCODE_WINDOWS
#define X88_ENCODING_SOURCE_SJIS
#define X88_ENCODING_GUI_SJIS

#else // WinGTK+ API

#define X88_PLATFORM_WINDOWS
#define X88_GUI_GTK
#define X88_GUI_GTK_USEIMAGE
#define X88_GUI_GTK_NOKEYRELEASE
#define X88_GUI_GTK_IGNORECHARRELEASE
#define X88_ENCODE_WINDOWS
#define X88_ENCODING_SOURCE_SJIS
#define X88_ENCODING_GUI_UTF8
#define X88_PRINTER_DRAW_CAIRO

#endif // using API for Windows

#else // UNIX Platform

#define X88_PLATFORM_UNIX
#define X88_GUI_GTK
#define X88_GUI_GTK_USEXWINDOW
#define X88_GUI_GTK_USEKEYMAP
#define X88_GUI_GTK_USEIM
#define X88_ENCODE_GTK
#define X88_ENCODING_SOURCE_UTF8
#define X88_ENCODING_GUI_UTF8
#define X88_PRINTER_DRAW_CAIRO

#endif // Platform

////////////////////////////////////////////////////////////
// include

// Standard Library

#ifdef X88_PLATFORM_WINDOWS

#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <direct.h>
#include <io.h>
#include <math.h>

#define strcasecmp	stricmp
#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define unlink		_unlink

//typedef signed char int8_t;
//typedef unsigned char uint8_t;
//typedef signed short int16_t;
//typedef unsigned short uint16_t;
//typedef signed long int32_t;
//typedef unsigned long uint32_t;

#elif defined(X88_PLATFORM_UNIX)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/mman.h>

#define _MAX_PATH	(_POSIX_PATH_MAX+1)

#endif // X88_PLATFORM

// GUI
#ifdef X88_GUI_WINDOWS

#define X88_BYTEORDER_LITTLE_ENDIAN

//#define _WINVER			0x0400
//#define _WIN32_WINNT	0x0400
//#define _WIN32_IE		0x0300
//
#include <windows.h>
#include <commctrl.h>
#include <prsht.h>
#include <imm.h>

// DirectX

//#define DIRECTDRAW_VERSION	0x0300
//#define DIRECTSOUND_VERSION	0x0300
//#define DIRECTINPUT_VERSION	0x0300
//
//#include <ddraw.h>
//#include <dsound.h>
//#include <dinput.h>

typedef HWND CX88WndHandle;

#elif defined(X88_GUI_GTK)

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pango/pango.h>
#include <cairo.h>
#include <cairo-pdf.h>

#define IDOK		GTK_RESPONSE_OK
#define IDCANCEL	GTK_RESPONSE_CANCEL
#define IDYES		GTK_RESPONSE_YES
#define IDNO		GTK_RESPONSE_NO

#ifdef X88_GUI_GTK_USEXWINDOW

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <gdk/gdkx.h>

#endif // X88_GUI_GTK_USEXWINDOW

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define X88_BYTEORDER_LITTLE_ENDIAN
#else // G_BYTE_ORDER != G_LITTLE_ENDIAN
#define X88_BYTEORDER_BIG_ENDIAN
#endif // G_BYTE_ORDER

typedef GtkWidget* CX88WndHandle;

#ifdef X88_PLATFORM_WINDOWS

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#define MSG				MSG_ERROR
#define LRESULT			LRESULT_ERROR
#define WPARAM			WPARAM_ERROR
#define LPARAM			LPARAM_ERROR
#define HWND			HWND_ERROR
#define HMENU			HMENU_ERROR
#define HICON			HICON_ERROR
#define HCURSOR			HCURSOR_ERROR
#define HACCEL			HACCEL_ERROR
#define HDC				HDC_ERROR
#define HGDIOBJ			HGDIOBJ_ERROR
#define HPEN			HPEN_ERROR
#define HBRUSH			HBRUSH_ERROR
#define HBITMAP			HBITMAP_ERROR
#define HFONT			HFONT_ERROR
#define HPALETTE		HPALETTE_ERROR
#define HENHMETAFILE	HENHMETAFILE_ERROR
#define HMETAFILE		HMETAFILE_ERROR
#define HMETAFILEPICT	HMETAFILEPICT_ERROR
#define HRGN			HRGN_ERROR
#define HIMC			HIMC_ERROR

#elif defined(X88_PLATFORM_UNIX)

#ifdef __CYGWIN__

#define X_LOCALE
#include <X11/Xlocale.h>
#undef setlocale

#endif // __CYGWIN__

#endif // X88_PLATFORM

#endif // X88_GUI

// STL

#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <queue>
#include <iterator>
#include <functional>
#include <algorithm>
#include <string>

////////////////////////////////////////////////////////////
// macro

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif // M_PI

#define min_value(a, b)	(((a) <= (b))? (a): (b))
#define max_value(a, b)	(((a) >= (b))? (a): (b))
