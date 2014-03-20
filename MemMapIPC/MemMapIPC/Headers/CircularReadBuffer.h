//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#ifndef __MemMapIPC_CircularReadBuffer_h__
#define __MemMapIPC_CircularReadBuffer_h__

#include "BasicTypes.h"
#include "CircularBuffer.h"

namespace MemMapIPC
{
    class cCircularReadBuffer : cCircularBuffer
    {
    public:

        cCircularReadBuffer();
        cCircularReadBuffer(U8 *bufferLocation, U64 bufferSize);

        U64 NumberOfBytesAvailableToRead();

        bool ReadData(U8 *dataSink, size_t numberOfBytes);
    };
}

#endif
