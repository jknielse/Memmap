//! Copyright 2013 Virtium Technology, Inc.
//! All rights reserved
//!

#ifndef __MemMapIPC_CircularBuffer_h__
#define __MemMapIPC_CircularBuffer_h__

#include "BasicTypes.h"

namespace MemMapIPC
{

    class cCircularBuffer
    {
    public:
        static const U64 RESERVE_SIZE;

    protected:
        cCircularBuffer();

        U64 m_BufferSize;
        U64 m_EffectiveBufferSize;
        U8 *m_Buffer;
        U8 *m_BufferReservePointer;

        //A circular buffer is a one-way communication entity. A producer makes data available on the buffer,
        //and a consumer reads that data.

        U64 *m_ProducerPointer;
        U64 *m_SecondProducerPointer;
        U64 *m_ConsumerPointer;
        U64 *m_SecondConsumerPointer;

        U8* ProducerMemoryAddress();
        U8* ConsumerMemoryAddress();

        void AdvanceProducer(U64 amount);
        void AdvanceConsumer(U64 amount);

    };
}

#endif