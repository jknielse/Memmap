//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#ifndef __MemMapIPC_MemMapEntity_h__
#define __MemMapIPC_MemMapEntity_h__

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/interprocess/windows_shared_memory.hpp"
#include "boost/interprocess/mapped_region.hpp"

#include "BasicTypes.h"
#include "CircularReadBuffer.h"
#include "CircularWriteBuffer.h"

namespace MemMapIPC
{
    class cEntity
    {
    protected:
        static const U64 KEY_SIZE_IN_BYTES = 64;

    public:
        void SetTimeout(double timeoutLimit);

        U64 NumberOfBytesAvailableToRead();
        U64 NumberOfBytesFreeToWrite();

        bool WriteData(U8 *dataSource, size_t numberOfBytes);
        bool ReadData(U8 *dataSink, size_t numberOfBytes);

        const Char* ReadKey();
        void WriteKey(Char key[cEntity::KEY_SIZE_IN_BYTES]);

        const boost::posix_time::ptime* GetServerTimestamp();
        const boost::posix_time::ptime* GetClientTimestamp();

    protected:

        virtual void UpdateTimestamp() = 0;

        cEntity();

        double m_TimeoutLimit;

        boost::interprocess::windows_shared_memory m_SharedMemoryObject;
        boost::interprocess::mapped_region m_MappedRegion;

        boost::posix_time::ptime* m_ServerTimestamp;
        boost::posix_time::ptime* m_ClientTimestamp;

        Char* m_Key;
        U64* m_BufferSize;

        cCircularReadBuffer m_ReadBuffer;
        cCircularWriteBuffer m_WriteBuffer;

        static const U64 RESERVE_SIZE;
    };
}

#endif