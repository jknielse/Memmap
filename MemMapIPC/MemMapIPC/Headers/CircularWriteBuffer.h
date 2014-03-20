//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#ifndef __MemMapIPC_CircularWriteBuffer_h__
#define __MemMapIPC_CircularWriteBuffer_h__

#include "BasicTypes.h"
#include "CircularBuffer.h"

namespace MemMapIPC
{
    class cCircularWriteBuffer : cCircularBuffer
    {
    public:

        cCircularWriteBuffer();
        cCircularWriteBuffer(U8 *bufferLocation, U64 bufferSize);

        U64 NumberOfBytesFreeToWrite();

        bool WriteData(U8 *dataSource, size_t numberOfBytes);

    };
}

#endif
