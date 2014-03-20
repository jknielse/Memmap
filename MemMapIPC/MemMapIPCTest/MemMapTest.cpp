#include "stdafx.h"
#include "CppUnitTest.h"

#include <thread>
#include <Windows.h>
#include <time.h>
#include <list>

#include "BasicTypes.h"
#include "Client.h"
#include "Server.h"
#include "Observer.h"

using namespace MemMapIPC;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define _CRT_SECURE_NO_WARNINGS

//2 GB
#define BENCHMARK_TOTAL_TRANSFER_SIZE ((U64)((U64)2*(U64)1024*(U64)1024*(U64)1024) - (U64)1)

//3 GB
#define VERIFICATION_TRANSFER_SIZE (U64)3*(U64)1024*(U64)1024*(U64)1024

//32 MB
#define TEST_BUFFER_SIZE 32*1024*1024
//The plus 3 is to make sure the chunk sizes will cause wraparound to be excercised
#define SMALL_CHUNK_SIZE TEST_BUFFER_SIZE/1024 + 3
#define BUFFER_SIZE_CHUNK_SIZE TEST_BUFFER_SIZE + 3
#define LARGE_CHUNK_SIZE TEST_BUFFER_SIZE*3 + 3

#define SPAM_TEST_NUMBER_OF_MEMMAPS 18
#define TEST_RESERVE_SIZE 100
#define MAX_MESSAGE_SIZE 200

namespace MemMapIPCTest
{
    TEST_CLASS(MemMapTest)
    {
    public:
        static wchar_t message[MAX_MESSAGE_SIZE];

        static std::string globalMessage;

        U8 testBuffer[TEST_BUFFER_SIZE];

        static void checkGlobalMessage()
        {
            if (!globalMessage.empty())
            {
                std::wstring widestringGlobalMessage(globalMessage.begin(),globalMessage.end());
                _swprintf(message, L"%s", widestringGlobalMessage.c_str());
                Assert::Fail(message, LINE_INFO());
            }
        }

        TEST_CLASS_INITIALIZE(ClassInitialization)
        {
            globalMessage = "";
        }

        TEST_CLASS_CLEANUP(ClassCleanup)
        {
        }

        TEST_METHOD_INITIALIZE(TestInitialization)
        {
            memset(testBuffer,0,TEST_BUFFER_SIZE);
            globalMessage = "";
        }

        TEST_METHOD_CLEANUP(TestCleanup)
        {
            checkGlobalMessage();
        }

        TEST_METHOD(MemMapObserverAllocation)
        {
            cServer server("MemTest",TEST_BUFFER_SIZE);
            cObserver observer("MemTest");

            //Check that we can actually call a method on this observer object
            observer.GetServerTimestamp();
        }

        TEST_METHOD(MemMapEntityTestNumberOfByteConsistensy)
        {
            cServer server("MemTest",TEST_BUFFER_SIZE);
            cClient client("MemTest");

            U64 numberOfBytesExchanged = 0;

            while(numberOfBytesExchanged < TEST_BUFFER_SIZE*3)
            {
                //The -100 is for the reserved space. Although we allocate
                //TEST_BUFFER_SIZE of memory, some of it is used for
                //book keeping, and we only expect the number of available bytes
                //to be accurate up to the usable buffer size.
                U64 writeSize = rand() % (TEST_BUFFER_SIZE - TEST_RESERVE_SIZE);
                bool test = server.WriteData(testBuffer, writeSize);
                if(!test)
                {
                    globalMessage = "cServer WriteData timed out!";
                }
                if (client.NumberOfBytesAvailableToRead() != writeSize)
                {
                    _swprintf(message, L"cClient did not have the same number of bytes available as cServer sent\n Write Size: %d\n Read Reported: %d",writeSize,client.NumberOfBytesAvailableToRead());
                    Assert::Fail(message, LINE_INFO());
                }
                test = client.WriteData(testBuffer, writeSize);
                if (!test)
                {
                    globalMessage = "cClient WriteData timed out";
                }
                if (server.NumberOfBytesAvailableToRead() != writeSize)
                {
                    _swprintf(message, L"cServer did not have the same number of bytes available as cClient sent");
                    Assert::Fail(message, LINE_INFO());
                }

                //Reset the read pointers
                test = server.ReadData(testBuffer, writeSize);
                if(!test)
                {
                    globalMessage = "cServer ReadData timed out!";
                }
                test = client.ReadData(testBuffer, writeSize);
                if(!test)
                {
                    globalMessage = "cClient ReadData timed out!";
                }

                numberOfBytesExchanged += writeSize;
            }
        }
        TEST_METHOD(MemMapEntityClientReadTimeout)
        {
            cServer server("MemTest",TEST_BUFFER_SIZE);
            cClient client("MemTest");

            bool result = client.ReadData(testBuffer,1);

            if (result)
            {
                _swprintf(message, L"cClient did not return false on a read timeout");
                Assert::Fail(message, LINE_INFO());
            }
        }

        TEST_METHOD(MemMapEntityClientWriteTimeout)
        {
            cServer server("MemTest",TEST_BUFFER_SIZE);
            cClient client("MemTest");

            bool result = client.WriteData(testBuffer,TEST_BUFFER_SIZE + 1);

            if (result)
            {
                _swprintf(message, L"cClient did not return false on a write timeout");
                Assert::Fail(message, LINE_INFO());
            }
        }

        TEST_METHOD(MemMapEntityServerReadTimeout)
        {
            cServer server("MemTest",TEST_BUFFER_SIZE);
            cClient client("MemTest");

            bool result = server.ReadData(testBuffer,1);

            if (result)
            {
                _swprintf(message, L"cServer did not return false on a read timeout");
                Assert::Fail(message, LINE_INFO());
            }
        }

        TEST_METHOD(MemMapEntityServerWriteTimeout)
        {
            cServer server("MemTest",TEST_BUFFER_SIZE);
            cClient client("MemTest");

            bool result = server.WriteData(testBuffer,TEST_BUFFER_SIZE + 1);

            if (result)
            {
                _swprintf(message, L"cServer did not return false on a write timeout");
                Assert::Fail(message, LINE_INFO());
            }
        }

        TEST_METHOD(MemMapServerSharedMemoryIDTest)
        {
            bool result = cObserver::IsSharedMemoryIDFree("ClearlyFreeValue, who would choose a string like this? Seriously.");
            if (!result)
            {
                _swprintf(message, L"cServer::IsSharedMemoryIDFree did not return true on a very obviously free shared memory ID");
                Assert::Fail(message, LINE_INFO());
            }

            cServer server("MemTest",TEST_BUFFER_SIZE);

            result = cObserver::IsSharedMemoryIDFree("MemTest");
            if (result)
            {
                _swprintf(message, L"cServer::IsSharedMemoryIDFree returned true when it should have returned false");
                Assert::Fail(message, LINE_INFO());
            }
        }

        //  ***************************************
        //  *                                     *
        //  *       R/W Verification Tests        *
        //  *                                     *
        //  ***************************************


        static void ServerThreadTwoWayVerifyMethod(U64 bufferSize, U64 chunkSize)
        {
            cServer server("Test", bufferSize);

            wchar_t message[MAX_MESSAGE_SIZE];
            long consumptionCounter = 0;
            long productionCounter = 0;

            U8 *producerData = new U8[chunkSize];
            U8 *simulatedBuffer = new U8[chunkSize];

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }


            //i.e. while either the server or the client process do not have all of the data that they need to exit
            while((consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)||(productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
            {
                //Check to see if the producer pointer is not going to run over the consumer pointer
                //It may seem silly to check both that it will not overrun the nextConsumerPointer as well as the consumer pointer,
                //but if the consumer pointer is midway through advancing when this check is made, this will ensure that we will not jump the gun.
                if ((server.NumberOfBytesFreeToWrite() >= chunkSize) && (productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = server.WriteData(producerData,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cServer WriteData timed out!";
                    }
                    productionCounter++;
                }

                //if there is a unit of data ready
                if ((server.NumberOfBytesAvailableToRead() >= chunkSize) && (consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = server.ReadData(simulatedBuffer,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cServer ReadData timed out!";
                    }
                    consumptionCounter++;

                    for (U64 i = 0; i < chunkSize; i++)
                    {
                        if(simulatedBuffer[i] != (U8)i)
                        {
                            globalMessage = "cServer did not return the same data that was passed into cClient";
                        }
                    }
                }
            }

            delete producerData;
            delete simulatedBuffer;
        }

        static void ClientThreadTwoWayVerifyMethod(U64 bufferSize, U64 chunkSize)
        {
            wchar_t message[MAX_MESSAGE_SIZE];
            long consumptionCounter = 0;
            long productionCounter = 0;

            U8 *producerData = new U8[chunkSize];
            U8 *simulatedBuffer = new U8[chunkSize];

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }

            cClient client("Test");


            while((consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)||(productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
            {

                //Check to see if the producer pointer is not going to run over the consumer pointer
                //It may seem silly to check both that it will not overrun the nextConsumerPointer as well as the consumer pointer,
                //but if the consumer pointer is midway through advancing when this check is made, this will ensure that we will not jump the gun.
                if ((client.NumberOfBytesFreeToWrite() >= chunkSize) && (productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = client.WriteData(producerData,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cClient WriteData timed out!";
                    }
                    productionCounter++;
                }

                //if there is a unit of data ready
                if ((client.NumberOfBytesAvailableToRead() >= chunkSize) && (consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = client.ReadData(simulatedBuffer,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cClient ReadData timed out!";
                    }
                    consumptionCounter++;

                    for (U64 i = 0; i < chunkSize; i++)
                    {
                        if(simulatedBuffer[i] != (U8)i)
                        {
                            globalMessage = "cClient did not return the same data that was passed into cServer";
                        }
                    }
                }
            }

            delete simulatedBuffer;
            delete producerData;
        }

        TEST_METHOD(MemMapEntitySmallChunkTwoWayReadWriteVerify)
        {
            std::thread serverThread(ServerThreadTwoWayVerifyMethod,TEST_BUFFER_SIZE,SMALL_CHUNK_SIZE);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(ClientThreadTwoWayVerifyMethod,TEST_BUFFER_SIZE,SMALL_CHUNK_SIZE);

            serverThread.join();
            clientThread.join();

            checkGlobalMessage();
        }

        //for the larger sizes of chunks (i.e. chunks that are larger than the buffer)
        //we can't do a two-way test because the threads will deadlock. Therefore we
        //need to do two one-way tests.

        static void ServerThreadProducerVerifyMethod(U64 bufferSize, U64 chunkSize)
        {
            cServer server("Test", bufferSize);

            long productionCounter = 0;

            U8 *producerData = new U8[chunkSize];

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }


            //i.e. while either the server or the client process do not have all of the data that they need to exit
            while(productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)
            {
                bool test = server.WriteData(producerData,chunkSize);
                if(!test)
                {
                    globalMessage = "cServer WriteData timed out!";
                }
                productionCounter++;
            }

            delete producerData;
        }

        static void ClientThreadConsumerVerifyMethod(U64 bufferSize, U64 chunkSize)
        {
            wchar_t message[MAX_MESSAGE_SIZE];
            long consumptionCounter = 0;

            U8 *simulatedBuffer = new U8[chunkSize];

            cClient client("Test");


            while(consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)
            {
                bool test = client.ReadData(simulatedBuffer,chunkSize);
                if(!test)
                {
                    globalMessage = "cClient ReadData timed out!";
                }
                consumptionCounter++;

                for (U64 i = 0; i < chunkSize; i++)
                {
                    if(simulatedBuffer[i] != (U8)i)
                    {
                        globalMessage = "cClient did not return the same data that was passed into cServer";
                    }
                }
            }

            delete simulatedBuffer;
        }

        static void ServerThreadConsumerVerifyMethod(U64 bufferSize, U64 chunkSize)
        {
            cServer server("Test", bufferSize);

            wchar_t message[MAX_MESSAGE_SIZE];
            long consumptionCounter = 0;

            U8 *simulatedBuffer = new U8[chunkSize];

            while(consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)
            {
                bool test = server.ReadData(simulatedBuffer,chunkSize);
                if (!test)
                {
                    globalMessage = "cServer ReadData timed out!";
                }
                consumptionCounter++;

                for (U64 i = 0; i < chunkSize; i++)
                {
                    if(simulatedBuffer[i] != (U8)i)
                    {
                        globalMessage = "cClient did not return the same data that was passed into cServer";
                    }
                }
            }

            delete simulatedBuffer;
        }

        static void ClientThreadProducerVerifyMethod(U64 bufferSize, U64 chunkSize)
        {

            long productionCounter = 0;

            U8 *producerData = new U8[chunkSize];

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }

            cClient client("Test");

            //i.e. while either the server or the client process do not have all of the data that they need to exit
            while(productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)
            {
                bool test = client.WriteData(producerData,chunkSize);
                if (!test)
                {
                    globalMessage = "cClient WriteData timed out!";
                }
                productionCounter++;
            }

            delete producerData;
        }

        TEST_METHOD(MemMapEntityLargeChunkServerProducerVerify)
        {
            std::thread serverThread(ServerThreadProducerVerifyMethod,TEST_BUFFER_SIZE,LARGE_CHUNK_SIZE);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(ClientThreadConsumerVerifyMethod,TEST_BUFFER_SIZE,LARGE_CHUNK_SIZE);

            serverThread.join();
            clientThread.join();

            checkGlobalMessage();
        }

        TEST_METHOD(MemMapEntityLargeChunkClientProducerVerify)
        {
            std::thread serverThread(ServerThreadConsumerVerifyMethod,TEST_BUFFER_SIZE,LARGE_CHUNK_SIZE);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(ClientThreadProducerVerifyMethod,TEST_BUFFER_SIZE,LARGE_CHUNK_SIZE);

            serverThread.join();
            clientThread.join();

            checkGlobalMessage();
        }

        TEST_METHOD(MemMapEntityBufferSizedChunkServerProducerVerify)
        {
            std::thread serverThread(ServerThreadProducerVerifyMethod,TEST_BUFFER_SIZE,BUFFER_SIZE_CHUNK_SIZE);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(ClientThreadConsumerVerifyMethod,TEST_BUFFER_SIZE,BUFFER_SIZE_CHUNK_SIZE);

            serverThread.join();
            clientThread.join();

            checkGlobalMessage();
        }

        TEST_METHOD(MemMapEntityBufferSizedChunkClientProducerVerify)
        {
            std::thread serverThread(ServerThreadConsumerVerifyMethod,TEST_BUFFER_SIZE,BUFFER_SIZE_CHUNK_SIZE);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(ClientThreadProducerVerifyMethod,TEST_BUFFER_SIZE,BUFFER_SIZE_CHUNK_SIZE);

            serverThread.join();
            clientThread.join();

            checkGlobalMessage();
        }

        //  ***************************************
        //  *                                     *
        //  *   Lopsided Production RW/Verify     *
        //  *                                     *
        //  ***************************************

        static void SlowServerThreadTwoWayVerifyMethod(U64 bufferSize, U64 chunkSize)
        {
            cServer server("Test", bufferSize);

            wchar_t message[MAX_MESSAGE_SIZE];
            long consumptionCounter = 0;
            long productionCounter = 0;

            U8 *producerData = new U8[chunkSize];
            U8 *simulatedBuffer = new U8[chunkSize];

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }


            //i.e. while either the server or the client process do not have all of the data that they need to exit
            while((consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)||(productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
            {
                Sleep(1);
                //Check to see if the producer pointer is not going to run over the consumer pointer
                //It may seem silly to check both that it will not overrun the nextConsumerPointer as well as the consumer pointer,
                //but if the consumer pointer is midway through advancing when this check is made, this will ensure that we will not jump the gun.
                if ((server.NumberOfBytesFreeToWrite() >= chunkSize) && (productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = server.WriteData(producerData,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cServer ReadData timed out!";
                    }
                    productionCounter++;
                }

                //if there is a unit of data ready
                if ((server.NumberOfBytesAvailableToRead() >= chunkSize) && (consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = server.ReadData(simulatedBuffer,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cServer ReadData timed out!";
                    }
                    consumptionCounter++;

                    for (U64 i = 0; i < chunkSize; i++)
                    {
                        if(simulatedBuffer[i] != (U8)i)
                        {
                            globalMessage = "cServer did not return the same data that was passed into cClient";
                        }
                    }
                }
            }

            delete producerData;
            delete simulatedBuffer;
        }

        static void SlowClientThreadTwoWayVerifyMethod(U64 bufferSize, U64 chunkSize)
        {
            wchar_t message[MAX_MESSAGE_SIZE];
            long consumptionCounter = 0;
            long productionCounter = 0;

            U8 *producerData = new U8[chunkSize];
            U8 *simulatedBuffer = new U8[chunkSize];

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }

            cClient client("Test");


            while((consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)||(productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
            {
                Sleep(1);
                //Check to see if the producer pointer is not going to run over the consumer pointer
                //It may seem silly to check both that it will not overrun the nextConsumerPointer as well as the consumer pointer,
                //but if the consumer pointer is midway through advancing when this check is made, this will ensure that we will not jump the gun.
                if ((client.NumberOfBytesFreeToWrite() >= chunkSize) && (productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = client.WriteData(producerData,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cClient WriteData timed out!";
                    }
                    productionCounter++;
                }

                //if there is a unit of data ready
                if ((client.NumberOfBytesAvailableToRead() >= chunkSize) && (consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = client.ReadData(simulatedBuffer,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cClient ReadData timed out!";
                    }
                    consumptionCounter++;

                    for (U64 i = 0; i < chunkSize; i++)
                    {
                        if(simulatedBuffer[i] != (U8)i)
                        {
                            globalMessage = "cClient did not return the same data that was passed into cServer";
                        }
                    }
                }
            }

            delete simulatedBuffer;
            delete producerData;
        }

        TEST_METHOD(MemMapEntitySmallChunkTwoWayReadWriteVerifyWithSlowerServer)
        {
            std::thread serverThread(SlowServerThreadTwoWayVerifyMethod,TEST_BUFFER_SIZE,SMALL_CHUNK_SIZE);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(ClientThreadTwoWayVerifyMethod,TEST_BUFFER_SIZE,SMALL_CHUNK_SIZE);

            serverThread.join();
            clientThread.join();

            checkGlobalMessage();
        }

        TEST_METHOD(MemMapEntitySmallChunkTwoWayReadWriteVerifyWithSlowerClient)
        {
            std::thread serverThread(ServerThreadTwoWayVerifyMethod,TEST_BUFFER_SIZE,SMALL_CHUNK_SIZE);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(SlowClientThreadTwoWayVerifyMethod,TEST_BUFFER_SIZE,SMALL_CHUNK_SIZE);

            serverThread.join();
            clientThread.join();

            checkGlobalMessage();
        }


        //  ***************************************
        //  *                                     *
        //  *        Memmap Spamming tests        *
        //  *                                     *
        //  ***************************************


        static void ServerThreadTwoWayVerifyMethodWithMemmapName(U64 bufferSize, U64 chunkSize, std::string memmapName)
        {
            cServer server(memmapName, bufferSize);

            wchar_t message[MAX_MESSAGE_SIZE];
            long consumptionCounter = 0;
            long productionCounter = 0;

            U8 *producerData = new U8[chunkSize];
            U8 *simulatedBuffer = new U8[chunkSize];

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }


            //i.e. while either the server or the client process do not have all of the data that they need to exit
            while((consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)||(productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
            {
                if (!globalMessage.empty())
                {
                    break;
                }
                //Check to see if the producer pointer is not going to run over the consumer pointer
                //It may seem silly to check both that it will not overrun the nextConsumerPointer as well as the consumer pointer,
                //but if the consumer pointer is midway through advancing when this check is made, this will ensure that we will not jump the gun.
                if ((server.NumberOfBytesFreeToWrite() >= chunkSize) && (productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = server.WriteData(producerData,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cServer WriteData timed out!";
                    }
                    productionCounter++;
                }

                //if there is a unit of data ready
                if ((server.NumberOfBytesAvailableToRead() >= chunkSize) && (consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = server.ReadData(simulatedBuffer,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cServer ReadData timed out!";
                    }
                    consumptionCounter++;

                    for (U64 i = 0; i < chunkSize; i++)
                    {
                        if(simulatedBuffer[i] != (U8)i)
                        {
                            globalMessage = "cServer did not return the same data that was passed into cClient";
                        }
                    }
                }
            }

            delete producerData;
            delete simulatedBuffer;
        }

        static void ClientThreadTwoWayVerifyMethodWithMemmapName(U64 bufferSize, U64 chunkSize, std::string memmapName)
        {
            wchar_t message[MAX_MESSAGE_SIZE];
            long consumptionCounter = 0;
            long productionCounter = 0;

            U8 *producerData = new U8[chunkSize];
            U8 *simulatedBuffer = new U8[chunkSize];

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }

            //Wait for the memory to be initialized by the server
            while(cObserver::IsSharedMemoryIDFree(memmapName))
            {
                Sleep(1);
            }

            cClient client(memmapName);


            while((consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize)||(productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
            {
                if (!globalMessage.empty())
                {
                    break;
                }

                //Check to see if the producer pointer is not going to run over the consumer pointer
                //It may seem silly to check both that it will not overrun the nextConsumerPointer as well as the consumer pointer,
                //but if the consumer pointer is midway through advancing when this check is made, this will ensure that we will not jump the gun.
                if ((client.NumberOfBytesFreeToWrite() >= chunkSize) && (productionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = client.WriteData(producerData,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cClient WriteData timed out!";
                    }
                    productionCounter++;
                }

                //if there is a unit of data ready
                if ((client.NumberOfBytesAvailableToRead() >= chunkSize) && (consumptionCounter < VERIFICATION_TRANSFER_SIZE/chunkSize))
                {
                    bool test = client.ReadData(simulatedBuffer,chunkSize);
                    if (!test)
                    {
                        globalMessage = "cClient ReadData timed out!";
                    }
                    consumptionCounter++;

                    for (U64 i = 0; i < chunkSize; i++)
                    {
                        if(simulatedBuffer[i] != (U8)i)
                        {
                            globalMessage = "cClient did not return the same data that was passed into cServer";
                        }
                    }
                }
            }

            delete simulatedBuffer;
            delete producerData;
        }

        TEST_METHOD(MemMapEntitySpamReadWriteVerify)
        {
            std::list<std::thread *> serverThreadList;
            std::list<std::thread *> clientThreadList;

            for ( int i = 0; i < SPAM_TEST_NUMBER_OF_MEMMAPS; i++)
            {
                std::string memmapNameString = std::string("memmap") + std::to_string(i);
                std::thread *serverThread = new std::thread(ServerThreadTwoWayVerifyMethodWithMemmapName,TEST_BUFFER_SIZE,SMALL_CHUNK_SIZE,memmapNameString);
                serverThreadList.push_front(serverThread);

                Sleep(1000);

                std::thread *clientThread = new std::thread(ClientThreadTwoWayVerifyMethodWithMemmapName,TEST_BUFFER_SIZE,SMALL_CHUNK_SIZE,memmapNameString);
                clientThreadList.push_front(clientThread);
            }

            //join and delete all of the server threads
            std::list<std::thread *>::iterator serverThreadListIterator = serverThreadList.begin();
            while(serverThreadListIterator != serverThreadList.end())
            {
                if((*serverThreadListIterator)->joinable())
                    (*serverThreadListIterator)->join();
                else
                    globalMessage = "A server thread was unjoinable. D:";
                delete (*serverThreadListIterator);
                checkGlobalMessage();
                ++serverThreadListIterator;
            }

            //join and delete all of the client threads
            std::list<std::thread *>::iterator clientThreadListIterator = clientThreadList.begin();
            while(clientThreadListIterator != clientThreadList.end())
            {
                if((*clientThreadListIterator)->joinable())
                    (*clientThreadListIterator)->join();
                else
                    globalMessage = "A client thread was unjoinable. D:";
                delete (*clientThreadListIterator);
                checkGlobalMessage();
                ++clientThreadListIterator;
            }

            checkGlobalMessage();
        }



        //  ***************************************
        //  *                                     *
        //  *          Benchmark Tests            *
        //  *                                     *
        //  ***************************************

        static void ProducerThreadMethod(U64 bufferSize, U64 chunkSize)
        {
            cServer server("Test", bufferSize);

            U8 *producerData = new U8[chunkSize];
            U64 productionCounter = 0;

            for (U64 i = 0; i < chunkSize; i++)
            {
                producerData[i] = (U8)i;
            }

            while(productionCounter < BENCHMARK_TOTAL_TRANSFER_SIZE/chunkSize)
            {
                bool test = server.WriteData(producerData,chunkSize);
                if(!test)
                {
                    globalMessage = "cServer WriteData timed out!";
                }
                productionCounter++;
            }

            delete producerData;
        }

        static void ConsumerThreadMethod(U64 bufferSize, U64 chunkSize)
        {
            wchar_t message[MAX_MESSAGE_SIZE];
            U64 consumptionCounter = 0;
            U8 *simulatedBuffer = new U8[chunkSize];

            cClient client("Test");

            clock_t startTime = clock();

            while(consumptionCounter < BENCHMARK_TOTAL_TRANSFER_SIZE/chunkSize)
            {
                client.ReadData(simulatedBuffer,chunkSize);
                consumptionCounter++;
            }

            clock_t endTime = clock();
            double elapsedTimeSeconds = ((double)(endTime - startTime))/CLOCKS_PER_SEC;

            double speedAchieved = ((double)((double)consumptionCounter * (chunkSize/(1024.0*1024.0))))/elapsedTimeSeconds;

            _swprintf(message, L"\nBuffer Size: %d \nChunk Size: %d \nBenchmark Speed: %g MB/s",bufferSize, chunkSize, speedAchieved);
            Logger::WriteMessage(message);

            delete simulatedBuffer;
        }

        void BufferChunksizeBenchmarkMethod(U64 bufferSize, U64 chunkSize)
        {
            std::thread serverThread(ProducerThreadMethod,bufferSize,chunkSize);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(ConsumerThreadMethod,bufferSize,chunkSize);

            serverThread.join();
            clientThread.join();
        }

        TEST_METHOD(MemMap_4K_Buffer_64_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(4*1024, 64);
        }

        TEST_METHOD(MemMap_16K_Buffer_64_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(16*1024, 64);
        }

        TEST_METHOD(MemMap_16K_Buffer_512_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(16*1024, 512);
        }

        TEST_METHOD(MemMap_16K_Buffer_8192_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(16*1024, 8192);
        }

        TEST_METHOD(MemMap_16K_Buffer_64000_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(16*1024, 64*1024);
        }

        TEST_METHOD(MemMap_32K_Buffer_64_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(32*1024, 64);
        }

        TEST_METHOD(MemMap_32K_Buffer_512_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(32*1024, 512);
        }

        TEST_METHOD(MemMap_32K_Buffer_8192_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(32*1024, 8192);
        }

        TEST_METHOD(MemMap_32M_Buffer_64_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(32*1024*1024, 64);
        }

        TEST_METHOD(MemMap_32M_Buffer_512_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(32*1024*1024, 512);
        }

        TEST_METHOD(MemMap_32M_Buffer_8192_Chunksize_Benchmark)
        {
            BufferChunksizeBenchmarkMethod(32*1024*1024, 8192);
        }
    };

    wchar_t MemMapTest::message[MAX_MESSAGE_SIZE];
    std::string MemMapTest::globalMessage;
}