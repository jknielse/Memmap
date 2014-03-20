//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#include <Windows.h>

#include <boost/timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

#include "Server.h"

using namespace boost::interprocess;

namespace MemMapIPC
{
    cServer::cServer()
    {
        m_SharedMemoryObject = windows_shared_memory();
        m_MappedRegion = mapped_region();
    }

    cServer::cServer(const String sharedMemoryID,const U64 bufferSize)
    {
        SetMemoryID(sharedMemoryID,bufferSize);
    }

    cServer::~cServer()
    {
    }

    void cServer::SetMemoryID(const String sharedMemoryID,const U64 bufferSize)
    {

        try
        {
            //Erase any previously shared memory
            shared_memory_object::remove(sharedMemoryID.c_str());

            //Create a shared memory object.
            m_SharedMemoryObject = windows_shared_memory(create_only, sharedMemoryID.c_str(), read_write, bufferSize * 2 + RESERVE_SIZE);

            //Map the whole shared memory in this process
            m_MappedRegion = mapped_region(m_SharedMemoryObject, read_write);
            int test = m_MappedRegion.get_page_size();

            //Make sure everything is in a clean state:
            memset(m_MappedRegion.get_address(),0,(size_t)(bufferSize*2 + RESERVE_SIZE));

            //First we'll set the timestamps and key:
            m_ServerTimestamp = (boost::posix_time::ptime *)m_MappedRegion.get_address();
            m_ClientTimestamp = (boost::posix_time::ptime *)m_ServerTimestamp + sizeof(boost::posix_time::ptime);

            m_Key = (Char *)m_ClientTimestamp + sizeof(boost::posix_time::ptime);
            m_BufferSize = (U64 *)(m_Key + sizeof(Char[cEntity::KEY_SIZE_IN_BYTES]));
            *m_BufferSize = bufferSize;

            //Set the buffer pointers to the two halves of the allocated shared memory.
            U8 *inboundBufferAddress = (U8 *)m_BufferSize + sizeof(U64);
            U8 *outboundBufferAddress = (inboundBufferAddress + bufferSize);

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

    void cServer::UpdateTimestamp()
    {
        *m_ServerTimestamp = boost::posix_time::second_clock::local_time();
    }
}
