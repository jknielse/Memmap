//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#ifndef __MemMapIPC_MemMapServer_h__
#define __MemMapIPC_MemMapServer_h__

#include <process.h>

#include "boost/interprocess/shared_memory_object.hpp"
#include "boost/interprocess/mapped_region.hpp"

#include "BasicTypes.h"
#include "CircularReadBuffer.h"
#include "CircularWriteBuffer.h"
#include "Entity.h"

namespace MemMapIPC
{
    class cServer : public cEntity
    {
    public:
        cServer();
        cServer(const String sharedMemoryID, const U64 bufferSize);
        ~cServer();

        void SetMemoryID(const String sharedMemoryID, const U64 bufferSize);

    private:

        void UpdateTimestamp();

    };
}

#endif
