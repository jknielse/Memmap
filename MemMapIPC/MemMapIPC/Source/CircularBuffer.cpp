//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#include "CircularBuffer.h"

namespace MemMapIPC
{
    //We need to reserve space for two U64s per buffer (one for the producer, and one for consumer)
    const U64 cCircularBuffer::RESERVE_SIZE = 4*sizeof(U64);

    cCircularBuffer::cCircularBuffer()
    {
    }

    U8* cCircularBuffer::ProducerMemoryAddress()
    {
        return (m_Buffer + (*m_ProducerPointer));
    }

    U8* cCircularBuffer::ConsumerMemoryAddress()
    {
        return (m_Buffer + (*m_ConsumerPointer));
    }

    void cCircularBuffer::AdvanceProducer(U64 amount)
    {
        U64 newProducerPointerValue = (*m_ProducerPointer) + amount;

        //if we've gone off the end of the buffer, wrap around.
        if (newProducerPointerValue >= m_EffectiveBufferSize)
        {
            newProducerPointerValue = newProducerPointerValue % m_EffectiveBufferSize;
        }

        (*m_ProducerPointer) = newProducerPointerValue;
        (*m_SecondProducerPointer) = newProducerPointerValue;
    }

    void cCircularBuffer::AdvanceConsumer(U64 amount)
    {
        U64 newConsumerPointerValue = (*m_ConsumerPointer) + amount;

        //if we've gone off the end of the buffer, wrap around.
        if (newConsumerPointerValue >= m_EffectiveBufferSize)
        {
            newConsumerPointerValue = newConsumerPointerValue % m_EffectiveBufferSize;
        }

        (*m_ConsumerPointer) = newConsumerPointerValue;
        (*m_SecondConsumerPointer) = newConsumerPointerValue;
    }
}
