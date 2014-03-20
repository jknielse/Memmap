//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#ifndef __MemMapIPC_MemMapObserver_h__
#define __MemMapIPC_MemMapObserver_h__

#include <process.h>

#include "boost/interprocess/shared_memory_object.hpp"
#include "boost/interprocess/mapped_region.hpp"

#include "BasicTypes.h"
#include "CircularReadBuffer.h"
#include "CircularWriteBuffer.h"
#include "Entity.h"

namespace MemMapIPC
{
    class cObserver : private cEntity
    {
    public:
        cObserver(const String sharedMemoryID);
        ~cObserver();

        static bool IsSharedMemoryIDFree(const String sharedMemoryID);

        const Char* ReadKey();

        const boost::posix_time::ptime* GetServerTimestamp();
        const boost::posix_time::ptime* GetClientTimestamp();

        void SetMemoryID(const String sharedMemoryID);
    private:
        void UpdateTimestamp();
        static const U64 TEST_MEM_SIZE;
    };
}

#endif
