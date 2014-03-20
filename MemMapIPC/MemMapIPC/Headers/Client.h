//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#ifndef __MemMapIPC_MemMapClient_h__
#define __MemMapIPC_MemMapClient_h__

#include <process.h>

#include "boost/interprocess/shared_memory_object.hpp"
#include "boost/interprocess/mapped_region.hpp"

#include "BasicTypes.h"
#include "CircularReadBuffer.h"
#include "CircularWriteBuffer.h"
#include "Entity.h"

namespace MemMapIPC
{
    class cClient : public cEntity
    {
    public:
        cClient();
        cClient(const String sharedMemoryID);
        ~cClient();

        void SetMemoryID(const String sharedMemoryID);
    private:

        void UpdateTimestamp();

    };
}

#endif
