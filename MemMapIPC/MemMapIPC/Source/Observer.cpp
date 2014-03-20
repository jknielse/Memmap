//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#include <Windows.h>

#include <boost/timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

#include "Observer.h"

using namespace boost::interprocess;

namespace MemMapIPC
{
    const U64 cObserver::TEST_MEM_SIZE = 1;

    cObserver::cObserver(const String sharedMemoryID)
    {
        SetMemoryID(sharedMemoryID);
    }

    cObserver::~cObserver()
    {
    }

    void cObserver::SetMemoryID(const String sharedMemoryID)
    {
        try
        {
            //Create a shared memory object.
            m_SharedMemoryObject = windows_shared_memory(open_only, sharedMemoryID.c_str(), read_write);

            //Map the whole shared memory in this process
            m_MappedRegion = mapped_region(m_SharedMemoryObject, read_write);

            size_t bufferSize = (m_MappedRegion.get_size() - RESERVE_SIZE)/2;

            //First we'll set the timestamps and key:
            m_ServerTimestamp = (boost::posix_time::ptime *)m_MappedRegion.get_address();
            m_ClientTimestamp = (boost::posix_time::ptime *)m_ServerTimestamp + sizeof(boost::posix_time::ptime);

            m_Key = (Char *)m_ClientTimestamp + sizeof(boost::posix_time::ptime);
            m_BufferSize = (U64 *)(m_Key + sizeof(Char[cEntity::KEY_SIZE_IN_BYTES]));

            //Set the buffer pointers to the two halves of the allocated shared memory.

            //Note that the outbound and inbound bufferes are the opposite of the server!
            //This means that our read buffer is the server's write buffer,
            //and our write buffer is the server's read buffer.
            U8 *outboundBufferAddress = (U8 *)m_BufferSize + sizeof(U64);
            U8 *inboundBufferAddress = outboundBufferAddress + bufferSize;

        }
        catch(interprocess_exception &ex)
        {
            shared_memory_object::remove(sharedMemoryID.c_str());
            std::cout << ex.what() << std::endl;
        }

    }

    bool cObserver::IsSharedMemoryIDFree(const String sharedMemoryID)
    {
        try
        {
            windows_shared_memory newObject(create_only, sharedMemoryID.c_str(), read_write, TEST_MEM_SIZE);
        }
        catch(interprocess_exception &ex)
        {
            if (ex.get_error_code() == already_exists_error)
            {
                return false;
            }
        }

        shared_memory_object::remove(sharedMemoryID.c_str());
        return true;
    }

    void cObserver::UpdateTimestamp()
    {
    }

    const Char* cObserver::ReadKey()
    {
        return m_Key;
    }

    const boost::posix_time::ptime* cObserver::GetServerTimestamp()
    {
        return m_ServerTimestamp;
    }

    const boost::posix_time::ptime* cObserver::GetClientTimestamp()
    {
        return m_ClientTimestamp;
    }

}
