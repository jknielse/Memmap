//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#include "CircularWriteBuffer.h"

namespace MemMapIPC
{

    cCircularWriteBuffer::cCircularWriteBuffer()
    {
    }

    cCircularWriteBuffer::cCircularWriteBuffer(U8 *bufferLocation, U64 bufferSize)
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

        *m_ProducerPointer = 0;
        *m_SecondProducerPointer = 0;
        *m_ConsumerPointer = 0;
        *m_SecondConsumerPointer = 0;
    }

    U64 cCircularWriteBuffer::NumberOfBytesFreeToWrite()
    {
        //We copy the contents of the outbound consumer pointer so that it wont change partway through the
        //execution of this function
        U64 nonvolatileConsumerPointer = (*m_SecondConsumerPointer);
        U64 nonvolatileConsumerCheckPointer = (*m_ConsumerPointer);

        while (nonvolatileConsumerPointer != nonvolatileConsumerCheckPointer)
        {
            nonvolatileConsumerPointer = (*m_SecondConsumerPointer);
            nonvolatileConsumerCheckPointer = (*m_ConsumerPointer);
        }

        if (nonvolatileConsumerPointer > (*m_ProducerPointer))
        {
            //If the outbound consumer is ahead of the outbound producer, then we know that
            //the number of bytes available to read is simply the difference of the two
            return nonvolatileConsumerPointer - (*m_ProducerPointer) - 1;
        }
        else
        {
            //If the outbound consumer is behind the outbound producer, then we need to
            //account for the wraparound:
            return nonvolatileConsumerPointer + m_EffectiveBufferSize - (*m_ProducerPointer) - 1;
        }
    }

    bool cCircularWriteBuffer::WriteData(U8 *dataSource, size_t numberOfBytes)
    {
        size_t numberOfBytesBeforeWraparound = (size_t)(m_EffectiveBufferSize - (*m_ProducerPointer));

        if (numberOfBytesBeforeWraparound >= numberOfBytes)
        {
            //If we won't hit the wraparound on this write, we can just write the data and advance the pointer:
            memcpy(ProducerMemoryAddress(), dataSource, numberOfBytes);
            AdvanceProducer(numberOfBytes);
            return true;
        }
        else
        {
            //If we will hit the wraparound, we need to write what we can, wrap around, and then write the rest:
            memcpy(ProducerMemoryAddress(), dataSource, numberOfBytesBeforeWraparound);
            AdvanceProducer(numberOfBytesBeforeWraparound);

            memcpy(ProducerMemoryAddress(), (dataSource + numberOfBytesBeforeWraparound), numberOfBytes - numberOfBytesBeforeWraparound);
            AdvanceProducer(numberOfBytes - numberOfBytesBeforeWraparound);
            return true;
        }
    }
}
