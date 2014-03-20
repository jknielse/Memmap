#include "stdafx.h"
#include "CppUnitTest.h"

#include "BasicTypes.h"
#include "CircularReadBuffer.h"
#include "CircularWriteBuffer.h"

using namespace MemMapIPC;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

//32 MB
#define TEST_BUFFER_SIZE 32*1024*1024
#define TEST_RESERVE_SIZE 100

namespace MemMapIPCTest
{
    TEST_CLASS(CircularBufferTest)
    {
    public:
        wchar_t message[200];

        U8 testBuffer[TEST_BUFFER_SIZE];
        U8 sharedBuffer[TEST_BUFFER_SIZE];

        TEST_CLASS_INITIALIZE(ClassInitialization)
        {
        }

        TEST_CLASS_CLEANUP(ClassCleanup)
        {
        }

        TEST_METHOD_INITIALIZE(TestInitialization)
        {
            //Make sure the shared buffer starts in a good state
            memset(sharedBuffer,0,TEST_BUFFER_SIZE);
        }

        TEST_METHOD_CLEANUP(TestCleanup)
        {
            //Nothing to do here yet
        }


        TEST_METHOD(CircularBufferTestNumberOfReadByteConsistensy)
        {
            cCircularWriteBuffer writeBuffer(sharedBuffer,TEST_BUFFER_SIZE);
            cCircularReadBuffer readBuffer(sharedBuffer,TEST_BUFFER_SIZE);

            U64 numberOfBytesExchanged = 0;

            while(numberOfBytesExchanged < TEST_BUFFER_SIZE*3)
            {
                //The -100 is for the reserved space. Although we allocate
                //TEST_BUFFER_SIZE of memory, some of it is used for
                //book keeping, and we only expect the number of available bytes
                //to be accurate up to the usable buffer size.
                U64 writeSize = rand() % (TEST_BUFFER_SIZE - TEST_RESERVE_SIZE);
                writeBuffer.WriteData(testBuffer, writeSize);
                if (readBuffer.NumberOfBytesAvailableToRead() != writeSize)
                {
                    _swprintf(message, L"readBuffer did not have the same number of bytes available as writeBuffer sent\nWrite Size: %d\nRead Reported: %d\nFailure occured after %d bytes were exchanged\n",writeSize,readBuffer.NumberOfBytesAvailableToRead(),numberOfBytesExchanged);
                    Assert::Fail(message, LINE_INFO());
                }
                readBuffer.ReadData(testBuffer,writeSize);

                numberOfBytesExchanged += writeSize;
            }
        }

        TEST_METHOD(CircularBufferTestNumberOfReadByteConsistensyAllocReversed)
        {
            //Test to make sure it doesn't matter which buffer gets allocated first
            cCircularReadBuffer readBuffer(sharedBuffer,TEST_BUFFER_SIZE);
            cCircularWriteBuffer writeBuffer(sharedBuffer,TEST_BUFFER_SIZE);

            U64 numberOfBytesExchanged = 0;

            while(numberOfBytesExchanged < TEST_BUFFER_SIZE*3)
            {
                //The -100 is for the reserved space. Although we allocate
                //TEST_BUFFER_SIZE of memory, some of it is used for
                //book keeping, and we only expect the number of available bytes
                //to be accurate up to the usable buffer size.
                U64 writeSize = rand() % (TEST_BUFFER_SIZE - TEST_RESERVE_SIZE);
                writeBuffer.WriteData(testBuffer, writeSize);
                if (readBuffer.NumberOfBytesAvailableToRead() != writeSize)
                {
                    _swprintf(message, L"readBuffer did not have the same number of bytes available as writeBuffer sent\nWrite Size: %d\nRead Reported: %d\nFailure occured after %d bytes were exchanged\n",writeSize,readBuffer.NumberOfBytesAvailableToRead(),numberOfBytesExchanged);
                    Assert::Fail(message, LINE_INFO());
                }
                readBuffer.ReadData(testBuffer,writeSize);

                numberOfBytesExchanged += writeSize;
            }
        }

        TEST_METHOD(CircularBufferTestNumberOfWriteByteConsistensy)
        {
            cCircularWriteBuffer writeBuffer(sharedBuffer,TEST_BUFFER_SIZE);
            cCircularReadBuffer readBuffer(sharedBuffer,TEST_BUFFER_SIZE);

            U64 numberOfBytesExchanged = 0;

            while(numberOfBytesExchanged < TEST_BUFFER_SIZE*3)
            {
                //The -100 is for the reserved space. Although we allocate
                //TEST_BUFFER_SIZE of memory, some of it is used for
                //book keeping, and we only expect the number of available bytes
                //to be accurate up to the usable buffer size.
                U64 writeSize = rand() % (TEST_BUFFER_SIZE - TEST_RESERVE_SIZE);
                U64 before = writeBuffer.NumberOfBytesFreeToWrite();
                writeBuffer.WriteData(testBuffer, writeSize);
                U64 after = writeBuffer.NumberOfBytesFreeToWrite();
                if (before-after != writeSize)
                {
                    _swprintf(message, L"Write buffer did not report having the correct number of write bytes remaining \nBefore Write: %d bytes free\nAfter Write: %d bytes free\nWrite Size: %d bytes\nFailure occured after %d bytes were exchanged\n",before,after,writeSize,numberOfBytesExchanged);
                    Assert::Fail(message, LINE_INFO());
                }
                readBuffer.ReadData(testBuffer,writeSize);

                numberOfBytesExchanged += writeSize;
            }
        }

        TEST_METHOD(CircularBufferTestTotalByteConsistensy)
        {
            cCircularWriteBuffer writeBuffer(sharedBuffer,TEST_BUFFER_SIZE);
            cCircularReadBuffer readBuffer(sharedBuffer,TEST_BUFFER_SIZE);

            U64 numberOfBytesExchanged = 0;

            while(numberOfBytesExchanged < TEST_BUFFER_SIZE*30)
            {
                //The -100 is for the reserved space. Although we allocate
                //TEST_BUFFER_SIZE of memory, some of it is used for
                //book keeping, and we only expect the number of available bytes
                //to be accurate up to the usable buffer size.
                U64 writeSize = rand() % (TEST_BUFFER_SIZE - TEST_RESERVE_SIZE);
                writeBuffer.WriteData(testBuffer, writeSize);
                U64 numberOfBytesToWrite = writeBuffer.NumberOfBytesFreeToWrite();
                U64 numberOfBytesToRead = readBuffer.NumberOfBytesAvailableToRead();
                U64 expectedTotal = TEST_BUFFER_SIZE - cCircularBuffer::RESERVE_SIZE - 1;
                if (numberOfBytesToWrite + numberOfBytesToRead != expectedTotal)
                {
                    _swprintf(message, L"Write buffer and read buffer did not report having the a number of bytes available that adds up to the buffer size minus the reserve size\nFailure occured after %d bytes were exchanged\n",numberOfBytesExchanged);
                    Assert::Fail(message, LINE_INFO());
                }
                readBuffer.ReadData(testBuffer,writeSize);

                numberOfBytesExchanged += writeSize;
            }
        }

        TEST_METHOD(CircularBufferTestNumberOfWriteByteConsistensyAllocReversed)
        {
            //Test to make sure it doesn't matter which buffer gets allocated first
            cCircularReadBuffer readBuffer(sharedBuffer,TEST_BUFFER_SIZE);
            cCircularWriteBuffer writeBuffer(sharedBuffer,TEST_BUFFER_SIZE);

            U64 numberOfBytesExchanged = 0;

            while(numberOfBytesExchanged < TEST_BUFFER_SIZE*3)
            {
                //The -100 is for the reserved space. Although we allocate
                //TEST_BUFFER_SIZE of memory, some of it is used for
                //book keeping, and we only expect the number of available bytes
                //to be accurate up to the usable buffer size.
                U64 writeSize = rand() % (TEST_BUFFER_SIZE - TEST_RESERVE_SIZE);
                U64 before = writeBuffer.NumberOfBytesFreeToWrite();
                writeBuffer.WriteData(testBuffer, writeSize);
                U64 after = writeBuffer.NumberOfBytesFreeToWrite();
                if (before-after != writeSize)
                {
                    _swprintf(message, L"Write buffer did not report having the correct number of write bytes remaining \nBefore Write: %d bytes free\nAfter Write: %d bytes free\nWrite Size: %d bytes\nFailure occured after %d bytes were exchanged\n",before,after,writeSize,numberOfBytesExchanged);
                    Assert::Fail(message, LINE_INFO());
                }
                readBuffer.ReadData(testBuffer,writeSize);

                numberOfBytesExchanged += writeSize;
            }
        }

        TEST_METHOD(CircularBufferReadWriteVerify)
        {
            cCircularWriteBuffer writeBuffer(sharedBuffer,TEST_BUFFER_SIZE);
            cCircularReadBuffer readBuffer(sharedBuffer,TEST_BUFFER_SIZE);

            //Check read/write sizes that will excercise the case where the data becomes split across the wrap-around
            //of the buffer.
            const int dataSize = 133;

            U8 dataSource[dataSize];
            U8 dataSink[dataSize];

            for (U64 testCounter = 0 ; testCounter < TEST_BUFFER_SIZE * 3; testCounter += dataSize)
            {
                for (U8 i = 0; i < dataSize; i++)
                {
                    dataSource[i] = rand();
                }

                writeBuffer.WriteData(dataSource,dataSize);
                readBuffer.ReadData(dataSink,dataSize);

                for (U8 i = 0; i < dataSize; i++)
                {
                    if(dataSource[i] != dataSink[i])
                    {
                        _swprintf(message, L"cCircularReadBuffer did not return the same data that was passed into cCircularReadBuffer");
                        Assert::Fail(message, LINE_INFO());
                    }
                }
            }
        }

        TEST_METHOD(CircularBufferReadWriteVerifyAllocReversed)
        {
            //Test to make sure it doesn't matter which buffer gets allocated first
            cCircularReadBuffer readBuffer(sharedBuffer,TEST_BUFFER_SIZE);
            cCircularWriteBuffer writeBuffer(sharedBuffer,TEST_BUFFER_SIZE);

            //Check read/write sizes that will excercise the case where the data becomes split across the wrap-around
            //of the buffer.
            const int dataSize = 133;

            U8 dataSource[dataSize];
            U8 dataSink[dataSize];

            for (U64 testCounter = 0 ; testCounter < TEST_BUFFER_SIZE * 3; testCounter += dataSize)
            {
                for (U8 i = 0; i < dataSize; i++)
                {
                    dataSource[i] = rand();
                }

                writeBuffer.WriteData(dataSource,dataSize);
                readBuffer.ReadData(dataSink,dataSize);

                for (U8 i = 0; i < dataSize; i++)
                {
                    if(dataSource[i] != dataSink[i])
                    {
                        _swprintf(message, L"cCircularReadBuffer did not return the same data that was passed into cCircularReadBuffer");
                        Assert::Fail(message, LINE_INFO());
                    }
                }
            }
        }
    };
}