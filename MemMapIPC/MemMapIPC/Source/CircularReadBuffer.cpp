//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!


#include <iostream>

#include "CircularReadBuffer.h"

namespace MemMapIPC
{
    cCircularReadBuffer::cCircularReadBuffer()
    {
    }

    cCircularReadBuffer::cCircularReadBuffer(U8 *bufferLocation, U64 bufferSize)
    {
        //Store the buffer information
        m_Buffer = bufferLocation;
        m_BufferSize = bufferSize;
        m_BufferReservePointer = m_Buffer + m_BufferSize - RESERVE_SIZE;
        m_EffectiveBufferSize = m_BufferSize - RESERVE_SIZE;

        //Initialize the producer and consumer pointers
        m_ProducerPointer =  (U64 *)(m_BufferReservePointer + (0*sizeof(U64)));
        m_SecondProducerPointer =  (U64 *)(m_BufferReservePointer + (1*sizeof(U64)));
        m_ConsumerPointer =  (U64 *)(m_BufferReservePointer + (2*sizeof(U64)));
        m_SecondConsumerPointer =  (U64 *)(m_BufferReservePointer + (3*sizeof(U64)));
    }

    U64 cCircularReadBuffer::NumberOfBytesAvailableToRead()
    {
        //We copy the contents of the inbound producer pointer so that it wont change partway through the
        //execution of this function
        U64 nonvolatileProducerPointer = (*m_SecondProducerPointer);
        U64 nonvolatileProducerPointerCheck = (*m_ProducerPointer);

        while (nonvolatileProducerPointer != nonvolatileProducerPointerCheck)
        {
            nonvolatileProducerPointer = (*m_SecondProducerPointer);
            nonvolatileProducerPointerCheck = (*m_ProducerPointer);
        }

        if (nonvolatileProducerPointer >= (*m_ConsumerPointer))
        {
            //If the inbound producer is ahead of the inbound consumer, then we know that
            //the number of bytes available to read is simply the difference of the two
            return nonvolatileProducerPointer - (*m_ConsumerPointer);
        }
        else
        {
            //If the inbound producer is behind the inbound consumer, then we need to
            //account for the wraparound:
            return nonvolatileProducerPointer + m_EffectiveBufferSize - (*m_ConsumerPointer);
        }
    }

    bool cCircularReadBuffer::ReadData(U8 * dataSink, size_t numberOfBytes)
    {

        size_t numberOfBytesBeforeWraparound = (size_t)(m_BufferSize - (*m_ConsumerPointer) - RESERVE_SIZE);

        if (numberOfBytesBeforeWraparound >= numberOfBytes)
        {
            //If we won't hit the wraparound on this write, we can just read the data and advance the pointer:
            memcpy(dataSink, ConsumerMemoryAddress(), numberOfBytes);
            AdvanceConsumer(numberOfBytes);
            return true;
        }
        else
        {
            //If we will hit the wraparound, we need to read what we can, wrap around, and then read the rest:
            memcpy(dataSink, ConsumerMemoryAddress(), numberOfBytesBeforeWraparound);
            AdvanceConsumer(numberOfBytesBeforeWraparound);

            memcpy((dataSink + numberOfBytesBeforeWraparound), ConsumerMemoryAddress(), numberOfBytes - numberOfBytesBeforeWraparound);
            AdvanceConsumer(numberOfBytes - numberOfBytesBeforeWraparound);
            return true;
        }
    }
}
