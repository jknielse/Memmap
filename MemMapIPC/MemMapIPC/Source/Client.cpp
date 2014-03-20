//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#include <Windows.h>

#include <boost/timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

#include "Client.h"

using namespace boost::interprocess;

namespace MemMapIPC
{
    cClient::cClient()
    {
    }

    cClient::cClient(const String sharedMemoryID)
    {
        SetMemoryID(sharedMemoryID);
    }

    cClient::~cClient()
    {
    }

    void cClient::SetMemoryID(const String sharedMemoryID)
    {
        try
        {
            //Create a shared memory object.
            m_SharedMemoryObject = windows_shared_memory(open_only, sharedMemoryID.c_str(), read_write);

            //Map the whole shared memory in this process
            m_MappedRegion = mapped_region(m_SharedMemoryObject, read_write);

            //First we'll set the timestamps and key:
            m_ServerTimestamp = (boost::posix_time::ptime *)m_MappedRegion.get_address();
            m_ClientTimestamp = (boost::posix_time::ptime *)m_ServerTimestamp + sizeof(boost::posix_time::ptime);

            m_Key = (Char *)m_ClientTimestamp + sizeof(boost::posix_time::ptime);
            m_BufferSize = (U64 *)(m_Key + sizeof(Char[cEntity::KEY_SIZE_IN_BYTES]));

            size_t bufferSize = *m_BufferSize;

            //Set the buffer pointers to the two halves of the allocated shared memory.

            //Note that the outbound and inbound bufferes are the opposite of the server!
            //This means that our read buffer is the server's write buffer,
            //and our write buffer is the server's read buffer.
            U8 *outboundBufferAddress = (U8 *)m_BufferSize + sizeof(U64);
            U8 *inboundBufferAddress = outboundBufferAddress + bufferSize;

            m_ReadBuffer = cCircularReadBuffer(inboundBufferAddress, bufferSize);
            m_WriteBuffer = cCircularWriteBuffer(outboundBufferAddress, bufferSize);
            UpdateTimestamp();

        }
        catch(interprocess_exception &ex)
        {
            std::cout << ex.what() << std::endl;
            throw ex;
        }

    }

    void cClient::UpdateTimestamp()
    {
        *m_ClientTimestamp = boost::posix_time::second_clock::local_time();
    }
}
