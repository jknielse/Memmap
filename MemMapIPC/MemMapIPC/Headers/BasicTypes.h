//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#ifndef __MemMapIPC_Basic_h__
#define __MemMapIPC_Basic_h__

#include <string>

namespace MemMapIPC
{
//--Basic data types
typedef char            Char;
typedef std::string     String;

typedef signed char      S8;
typedef unsigned char    U8;
typedef U8               Byte;

typedef signed short     S16;
typedef unsigned short   U16;

typedef signed long      S32;
typedef unsigned long    U32;

//We may need to point these at __int32s for the purposes of thread synchronization
//on 32 bit machines.
typedef signed __int64   S64;
typedef unsigned __int64 U64;

typedef float            F32;
typedef double           F64;
//--

}//end namespace


#endif
