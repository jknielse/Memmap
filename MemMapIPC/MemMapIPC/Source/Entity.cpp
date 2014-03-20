//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#include <boost/timer.hpp>
#include <iostream>

#include "Entity.h"

using namespace boost::interprocess;

namespace MemMapIPC
{
    //We need to reserve space for two timestamps and a string. The two timestamps are the
    //server and client timestamps respectively, and the char* is there for storing an identification
    //string.
    const U64 cEntity::RESERVE_SIZE = 2*sizeof(boost::posix_time::ptime) + sizeof(Char[64]) + sizeof(U64);

    //The timeout limit is the amount of time that the client has to hang before
    //cEntity will decide that they are disconnected.
    cEntity::cEntity()
    {
        m_TimeoutLimit = 120.0;
    }

    void cEntity::SetTimeout(double timoutLimit)
    {
        m_TimeoutLimit = timoutLimit;
    }

    U64 cEntity::NumberOfBytesFreeToWrite()
    {
        return m_WriteBuffer.NumberOfBytesFreeToWrite();
    }

    U64 cEntity::NumberOfBytesAvailableToRead()
    {
        return m_ReadBuffer.NumberOfBytesAvailableToRead();
    }

    bool cEntity::WriteData(U8 *dataSource, size_t numberOfBytes)
    {
        UpdateTimestamp();

        if (m_WriteBuffer.NumberOfBytesFreeToWrite() < numberOfBytes)
        {
            boost::timer timeoutTimer;
            size_t numberOfBytesWritten = 0;
            //While there hasn't been a timeout, and there's still data to write
            while ((timeoutTimer.elapsed() < m_TimeoutLimit) && (numberOfBytesWritten != numberOfBytes))
            {
                //If the write buffer has more room for data
                U64 freeBytes = m_WriteBuffer.NumberOfBytesFreeToWrite();
                if (0 < freeBytes)
                {
                    //Since the client is clearly still reading data, we can reset the timeout timer.
                    timeoutTimer.restart();
                    if (freeBytes > (numberOfBytes - numberOfBytesWritten))
                    {
                        m_WriteBuffer.WriteData(dataSource + numberOfBytesWritten, (numberOfBytes - numberOfBytesWritten));
                        numberOfBytesWritten = numberOfBytes;
                    }
                    else
                    {
                        m_WriteBuffer.WriteData(dataSource + numberOfBytesWritten, freeBytes);
                        numberOfBytesWritten += freeBytes;
                    }
                }
            }

            if (numberOfBytesWritten == numberOfBytes)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return m_WriteBuffer.WriteData(dataSource, numberOfBytes);
        }
    }

    bool cEntity::ReadData(U8 *dataSink, size_t numberOfBytes)
    {
        UpdateTimestamp();
        if (m_ReadBuffer.NumberOfBytesAvailableToRead() < numberOfBytes)
        {
            boost::timer timeoutTimer;
            size_t numberOfBytesRead = 0;
            //While there hasn't been a timeout, and there's still data to read
            while ((timeoutTimer.elapsed() < m_TimeoutLimit) && (numberOfBytesRead != numberOfBytes))
            {
                //If the write buffer has more room for data
                if (U64 freeBytes = m_ReadBuffer.NumberOfBytesAvailableToRead())
                {
                    //Since the server is clearly still writing data, we can reset the timeout timer.
                    timeoutTimer.restart();
                    if (freeBytes > (numberOfBytes - numberOfBytesRead))
                    {
                        m_ReadBuffer.ReadData(dataSink + numberOfBytesRead, (numberOfBytes - numberOfBytesRead));
                        numberOfBytesRead = numberOfBytes;
                    }
                    else
                    {
                        m_ReadBuffer.ReadData(dataSink + numberOfBytesRead, freeBytes);
                        numberOfBytesRead += freeBytes;
                    }
                }
            }

            if (numberOfBytesRead == numberOfBytes)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return m_ReadBuffer.ReadData(dataSink, numberOfBytes);
        }
    }

    const Char* cEntity::ReadKey()
    {
        return m_Key;
    }

    void cEntity::WriteKey(Char key[cEntity::KEY_SIZE_IN_BYTES])
    {
        memcpy(m_Key,key,sizeof(Char[cEntity::KEY_SIZE_IN_BYTES]));
    }

    const boost::posix_time::ptime* cEntity::GetServerTimestamp()
    {
        return m_ServerTimestamp;
    }

    const boost::posix_time::ptime* cEntity::GetClientTimestamp()
    {
        return m_ClientTimestamp;
    }
}
