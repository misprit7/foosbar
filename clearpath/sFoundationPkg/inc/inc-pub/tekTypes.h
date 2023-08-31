//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/tekTypes.h $
// $Revision: 77 $ $Date: 12/09/16 4:08p $
// $Workfile: tekTypes.h $
//
// NAME
//		tekTypes.h
//
// DESCRIPTION:
/** \file
	\brief Standardized types for Windows, QNX, Linux and DSP applications to 
	insure properly sized data items across platforms.

**/
// CREATION DATE:
//		05/17/2007 15:13:46
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2007-2013 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************
#ifndef __TEKTYPES_H__
#define	__TEKTYPES_H__

// Standard lib definitions
#include <limits.h>
#include <stdint.h>


#if defined(_WIN32)||defined(_WIN64)&&!defined(__ASM_HEADER__)
	#include <tchar.h>
#endif

#if !defined(__ASM_HEADER__)
#ifdef __TI_COMPILER_VERSION__
	#include "IQmathLib.h"
#endif

#ifndef ASM_CONST
typedef unsigned long asmConst;
#define ASM_CONST(x) ((unsigned long)(&x))
#endif

#endif

//||||||||||||||||||||||||||||||||||||
// Linux helper defines 
//||||||||||||||||||||||||||||||||||||


#if !(defined(_WIN32)||defined(_WIN64))&&!defined(__ASM_HEADER__)
	#ifndef _CRT_WARN
	#include <stdio.h>
	#endif
	#ifdef _DEBUG
			#define _RPT0(f,a0) fprintf(f,a0)
			#define _RPT1(f,a0,a1) fprintf(f,a0,a1)
			#define _RPT2(f,a0,a1,a2) fprintf(f,a0,a1,a2)
			#define _RPT3(f,a0,a1,a2,a3) fprintf(f,a0,a1,a2,a3)
			#define _RPT4(f,a0,a1,a2,a3,a4) fprintf(f,a0,a1,a2,a3,a4)
			#define _RPT5(f,a0,a1,a2,a3,a4,a5) fprintf(f,a0,a1,a2,a3,a4,a5)
			#define _RPTF0(f,a0) fprintf(f,a0)
			#define _RPTF1(f,a0,a1)fprintf(f,a0,a1)
			#define _RPTF2(f,a0,a1,a2) fprintf(f,a0,a1,a2)
	#else
			#define _RPT0(f,a0)
			#define _RPT1(f,a0,a1)
			#define _RPT2(f,a0,a1,a2)
			#define _RPT3(f,a0,a1,a2,a3)
			#define _RPT4(f,a0,a1,a2,a3,a4)
			#define _RPT5(f,a0,a1,a2,a3,a4,a5)
			#define _RPTF0(f,a0)
			#define _RPTF1(f,a0,a1)
			#define _RPTF2(f,a0,a1,a2)
	#endif
	// Allow project re-direct
	#ifndef _CRT_WARN
		#define _CRT_WARN stderr
	#endif
	#ifndef _CRT_ERROR
		#define _CRT_ERROR stderr
	#endif
	#ifndef _CRT_ASSERT
		#define _CRT_ASSERT stdout
	#endif
#endif


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TI header types mapped to platform equivalents.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// This header will implement TI header typedefs if not included
#if !(defined(DSP28_DATA_TYPES) || defined(_TI_STD_TYPES))
	typedef int16_t             int16;
	typedef int32_t             int32;
	typedef uint16_t            Uint16;
	typedef uint32_t            Uint32;
	typedef int					Int;
	typedef unsigned			Uns;
	typedef char            	*String;
	typedef void           		*Ptr;
	#if !defined(DSP28_DATA_TYPES)
		#define DSP28_DATA_TYPES
	#endif
	#if !defined(_TI_STD_TYPES)
		#define _TI_STD_TYPES
	#endif
#endif

typedef int_least8_t		int8;
typedef uint_least8_t		Uint8;
typedef uint64_t            Uint64;
typedef int64_t         	int64;
typedef float           	float32;
typedef long double     	float64;
/// Bit Field Type
typedef Uint16 BFld;


#ifndef FAR
	#define FAR
#endif


#ifndef INT16_MAX
#define INT16_MAX 32767
#endif

#ifndef INT32_MAX
#define INT32_MAX 0x7FFFFFFF
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Backward compatible definitions to ControlPoint library
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef Uint32 nodeparam;		// Node parameter

typedef Uint16 nodebool;		// Node boolean (VB boolean is 16-bits)
typedef Uint16 nodeushort;
typedef int16 nodeshort;
typedef Uint32 nodeulong;		// Unsigned 32-bit item
typedef int32 nodelong;			// Signed 32-bit item

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// String and character definitions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined(_WIN32)||defined(_WIN64)
	typedef char nodechar;			// "char" type
	typedef char nodeuchar;
	typedef char *nodeANSIstr;
	typedef wchar_t* nodeUNIstr;	// Unicode string
#else
// ANSI style 8-bit character. Maybe >8 bits if platform does not support
// octet addressing.
	#ifdef __TMS470__
		typedef int16 nodechar;		// "char" type
		typedef int16 nodeuchar;	// A character/extra attributes
	#else
#if defined(__linux__)
		typedef char nodechar;		// "char" type
		typedef char nodeuchar;		// A character
#else
		typedef int8 nodechar;		// "char" type
		typedef int8 nodeuchar;		// A character
#endif
	#endif
	typedef char *nodeANSIstr;
	typedef nodechar* nodeUNIstr;   // Unicode string
	// Define Windows normal macro
	#ifndef MAX_PATH
	#define MAX_PATH 260
	#endif
#endif
typedef nodechar *nodestr;			// String of nodechars


// Basic link character which includes out of band
// values such as break detected.
typedef Uint16 linkChar;	

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Serial Classes assume Windows standard sizes, assign to our generic typedefs
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if !(defined(_WIN32)||defined(_WIN64))
	#if defined(__cplusplus) && !defined(BOOL)
		typedef bool BOOL;
	#else
		typedef int16 BOOL;
	#endif
	typedef int32_t LONG;
	typedef int16_t WORD;
	typedef int32_t DWORD;
	typedef int64_t LONGLONG;
	typedef uint32_t ULONG;
	typedef uint16_t UINT;
	typedef const nodechar * LPCTSTR;

	#define _T (char *) 
#endif

// Quad Word Container for grabbing values at various
// 16-bit locations.
/// \cond	INTERNAL_DOC
#if defined(__TMS320C2000__) || defined(__PICCOLO)
typedef union _qTainer {
	int64	i64;
	struct _i32 {
		Uint32	lsb;
		int32	msb;
	} i32;
	struct _i16 {
		Uint16	lsb;
		Uint16  msb;
		Uint16	usb;
		int16   vsb;
	} i16;
} qTainer, nodequad;
// Long Word Container for grabbing values at various
// 16-bit locations.
typedef union _lTainer {
	int32	i32;
	struct {
		Uint16	lsb;
		int16   msb;
	} i16;
} lTainer;
#endif
/// \endcond

#if !defined(__ASM_HEADER__)
	#if defined(__TMS320C2000__) || defined(__PICCOLO)
		// DSP architectures
		#define OCTET_SIZE(x) (sizeof(x)*2)
	#else
		// Byte oriented architectures
		#define OCTET_SIZE(x) sizeof(x)
	#endif
#endif
typedef int16					tBOOL;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// IQmath mappings
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Short signed fixed-point values
typedef int16					_iq15_s;
typedef int16					_iq14_s;
typedef int16					_iq13_s;
typedef int16					_iq12_s;
typedef int16					_iq11_s;
typedef int16					_iq10_s;
typedef int16					_iq9_s;
typedef int16					_iq8_s;
typedef int16					_iq7_s;
typedef int16					_iq6_s;
typedef int16					_iq5_s;
typedef int16					_iq4_s;
typedef int16					_iq3_s;
typedef int16					_iq2_s;
typedef int16					_iq1_s;

// Short unsigned fixed-point values
typedef Uint16					_iq16_su;
typedef Uint16					_iq15_su;
typedef Uint16					_iq14_su;
typedef Uint16					_iq13_su;
typedef Uint16					_iq12_su;
typedef Uint16					_iq11_su;
typedef Uint16					_iq10_su;
typedef Uint16					_iq9_su;
typedef Uint16					_iq8_su;
typedef Uint16					_iq7_su;
typedef Uint16					_iq6_su;
typedef Uint16					_iq5_su;
typedef Uint16					_iq4_su;
typedef Uint16					_iq3_su;
typedef Uint16					_iq2_su;
typedef Uint16					_iq1_su;

typedef int32					_iq31;					// (1.31) fixed-point number
// Long fixed-point values
typedef int64				    _iq17_l;				// (47.17) variable [15.17 as LSBs]
typedef int64					_iq27_l;
typedef int64					_iq34_l;

#if !defined(__ASM_HEADER__)
// Create missing long Q constants
#define	  _IQ31(A)		  (int32) ((A) * 2147483648.0L)

// Create short Q constants
#define   _IQ15_S(A)      (int16) ((A) * 32768.0L)
#define   _IQ14_S(A)      (int16) ((A) * 16384.0L)
#define   _IQ13_S(A)      (int16) ((A) * 8192.0L)
#define   _IQ12_S(A)      (int16) ((A) * 4096.0L)
#define   _IQ11_S(A)      (int16) ((A) * 2048.0L)
#define   _IQ10_S(A)      (int16) ((A) * 1024.0L)
#define   _IQ9_S(A)       (int16) ((A) * 512.0L)
#define   _IQ8_S(A)       (int16) ((A) * 256.0L)
#define   _IQ7_S(A)       (int16) ((A) * 128.0L)
#define   _IQ6_S(A)       (int16) ((A) * 64.0L)
#define   _IQ5_S(A)       (int16) ((A) * 32.0L)
#define   _IQ4_S(A)       (int16) ((A) * 16.0L)
#define   _IQ3_S(A)       (int16) ((A) * 8.0L)
#define   _IQ2_S(A)       (int16) ((A) * 4.0L)
#define   _IQ1_S(A)       (int16) ((A) * 2.0L)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef VB_TRUE
#define VB_TRUE ((nodebool)-1)
#endif

#ifndef VB_FALSE
#define VB_FALSE ((nodebool)0)
#endif

#ifndef NULL 
#define NULL 0
#endif

#ifndef NULL_FUNC 
#define NULL_FUNC ((void (*)())0)
#endif

#ifndef __cplusplus
typedef FAR void *romPtr;
#else
typedef void *romPtr;
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Linkages and calling convention definitions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if !(defined(_WIN32)||defined(_WIN64)) || defined(__ASM_HEADER__) || defined(__TI_COMPILER_VERSION__)
	#ifndef EXTERN_C
		#ifdef __cplusplus
			#define EXTERN_C extern "C"
		#else
			#define EXTERN_C extern
		#endif
	#endif	// EXTERN_C
	// Ignore on non-Windows platforms
	#ifndef MN_EXPORT
		#define MN_EXPORT
	#endif
	#ifndef MN_DECL
		#define MN_DECL
	#endif
	#ifndef CALLBACK 
		#define CALLBACK
	#endif
	typedef int16 WCHAR;
#else
	// Windows C DLL Linkage hack
	#define	MN_DECL __stdcall
	#ifdef _DLL
		#define MN_EXPORT __declspec(dllexport)
	#else
		#define MN_EXPORT __declspec(dllimport)
	#endif

	#ifndef CALLBACK
		#define CALLBACK __stdcall
	#endif
	typedef wchar_t WCHAR;
#endif

// prevent declaration type issues with shared files
#if defined(__ASM_HEADER__)
#undef EXTERN_C
#define EXTERN_C
#endif
// Function signature macro defines
#if defined(_MSC_VER)
#define _TEK_FUNC_SIG_	__FUNCSIG__
#elif defined(__GNUC__)
#define _TEK_FUNC_SIG_	__PRETTY_FUNCTION__
#endif
// Useful for checking constant assertions
#ifdef __TI_COMPILER_VERSION__
	#define STATIC_ASSERT(e) { extern char assert_static__[(e) ? 1 : -1]; }
#else
	#define STATIC_ASSERT(e)
#endif
// Useful macros that generate good code
#if !defined(_MIN) && !defined(__ASM_HEADER__)
	#define _MIN(a,b) (a < b) ? a : b
#endif

#if !defined(_MAX) && !defined(__ASM_HEADER__)
	#define _MAX(a,b) (a > b) ? a : b
#endif

// Packing control to force structure alignments to byte boundaries
// Order of definitions is important for cases where multiple conditions may be true
#ifdef PACK_BYTES
#error "PACK_BYTES already defined"
#endif

#ifdef __DOXYGEN__
#define PACK_BYTES
#endif

#if (defined(__TI_COMPILER_VERSION__) || defined(__ASM_HEADER__)) && !defined(PACK_BYTES)
#define PACK_BYTES
#endif

#if defined(_MSC_VER) && !defined(PACK_BYTES)
#ifdef __cplusplus_cli
#define PACK_BYTES
#else
#define PACK_BYTES 
#endif
#endif

#if defined(__GNUC__) && !defined(PACK_BYTES)
#define PACK_BYTES __attribute__((aligned(1)))
#endif

#ifndef PACK_BYTES
// Make sure PACK_BYTES has been explicitly defined
#error "Please define PACK_BYTES for this platform"
// Suppress additional errors for PACK_BYTES not being known by defining it to do nothing
#define PACK_BYTES
#endif

//// Windows STL exports
//#if defined(_MSC_VER)
//#ifdef EXP_STL
//#    define DECLSPECIFIER __declspec(dllexport)
//#    define EXPIMP_TEMPLATE
//#else
//#    define DECLSPECIFIER __declspec(dllimport)
//#    define EXPIMP_TEMPLATE extern
//#endif
//#else
//	// Non-Windows
//#    define DECLSPECIFIER
//#    define EXPIMP_TEMPLATE
//#endif

// CALLBACK DEFINITION
#if defined(_WIN32)||defined(_WIN64)
#define nodeCallback __stdcall 
#else
#define nodeCallback
#endif


#endif	// __TEKTYPES_H__
//=============================================================================
//	END OF FILE tekTypes.h
//=============================================================================
