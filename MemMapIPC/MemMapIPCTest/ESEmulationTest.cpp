#include "stdafx.h"
#include "CppUnitTest.h"

#include <thread>
#include <Windows.h>
#include <time.h>
#include <list>
#include <exception>
#include <direct.h>

#include "BasicTypes.h"
#include "Client.h"
#include "Server.h"
#include "Observer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace MemMapIPC;

#define MAX_MESSAGE_SIZE 200
//16 KB
#define TEST_MEMMAP_BUFFER_SIZE 160*1024
//16 MB
#define TEST_ES_BUFFER_SIZE 16*1024*1024
//8 KB
#define INITIAL_PACKAGE_SIZE 8*1024


#define NUMBER_OF_TEST_TRANSFERS 500

namespace MemMapIPCTest
{
    TEST_CLASS(ESEmulationTest)
    {
    public:
        static wchar_t message[MAX_MESSAGE_SIZE];

        static std::string globalMessage;

        static void checkGlobalMessage()
        {
            if (!globalMessage.empty())
            {
                std::wstring widestringGlobalMessage(globalMessage.begin(),globalMessage.end());
                _swprintf(message, L"%s", widestringGlobalMessage.c_str());
                Assert::Fail(message, LINE_INFO());
            }
        }

        TEST_METHOD_INITIALIZE(TestInitialization)
        {
            globalMessage = "";
        }

        static void SimpleWriteCommandESThread()
        {
            cServer server("ESTest",TEST_MEMMAP_BUFFER_SIZE);
            U8* buffer = new U8[TEST_ES_BUFFER_SIZE];

            int packageCounter = 0;

            while ((packageCounter < NUMBER_OF_TEST_TRANSFERS) && globalMessage.empty())
            {
                if(server.NumberOfBytesAvailableToRead() >= INITIAL_PACKAGE_SIZE)
                {
                    bool test = server.ReadData((U8 *)buffer, INITIAL_PACKAGE_SIZE);

                    if(!test)
                    {
                        globalMessage = "ES Read Initial Package Timed Out!";
                        break;
                    }

                    U64 transferSize = *((U64*) buffer);

                    if(transferSize > TEST_ES_BUFFER_SIZE)
                    {
                        globalMessage = "Requested transfer size was too large";
                        break;
                    }

                    test = server.ReadData((U8 *)buffer, transferSize);
                    if(!test)
                    {
                        globalMessage = "ES Read Subsequent Data Timed Out!";
                        break;
                    }
                    packageCounter++;
                }
            }

            delete buffer;
        }

        static void SimpleWriteCommandSnareThread()
        {
            cClient client("ESTest");
            U8* buffer = new U8[TEST_ES_BUFFER_SIZE];

            int packageCounter = 0;

            for (U64 i = 0; i < TEST_ES_BUFFER_SIZE; i++)
            {
                buffer[i] = 0;
            }

            while ((packageCounter < NUMBER_OF_TEST_TRANSFERS) && globalMessage.empty())
            {
                U64 testSize = rand() % TEST_ES_BUFFER_SIZE;
                *((U64 *)buffer) = testSize;

                bool test = client.WriteData((U8 *)buffer, INITIAL_PACKAGE_SIZE);

                if(!test)
                {
                    globalMessage = "Snare Write Initial Package Timed Out!";
                        break;
                }

                test = client.WriteData((U8 *)buffer, testSize);

                if(!test)
                {
                    globalMessage = "Snare Write Subsequent Data Timed Out!";
                        break;
                }
                packageCounter++;
            }

            delete buffer;
        }

        TEST_METHOD(SimpleWriteCommand)
        {
            std::thread serverThread(SimpleWriteCommandESThread);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(SimpleWriteCommandSnareThread);

            serverThread.join();
            clientThread.join();
            checkGlobalMessage();
        }


        static void SimpleReadCommandESThread()
        {
            cServer server("ESTest",TEST_MEMMAP_BUFFER_SIZE);
            U8* buffer = new U8[TEST_ES_BUFFER_SIZE];

            for (U64 i = 0; i < TEST_ES_BUFFER_SIZE; i++)
            {
                buffer[i] = 0;
            }

            int packageCounter = 0;

            while ((packageCounter < NUMBER_OF_TEST_TRANSFERS) && globalMessage.empty())
            {
                if(server.NumberOfBytesAvailableToRead() >= INITIAL_PACKAGE_SIZE)
                {
                    bool test = server.ReadData((U8 *)buffer, INITIAL_PACKAGE_SIZE);

                    if(!test)
                    {
                        globalMessage = "ES Read Initial Package Timed Out!";
                        break;
                    }

                    U64 transferSize = *((U64*) buffer);

                    if(transferSize > TEST_ES_BUFFER_SIZE)
                    {
                        globalMessage = "Requested transfer size was too large";
                        break;
                    }

                    test = server.WriteData((U8 *)buffer, transferSize);
                    if(!test)
                    {
                        globalMessage = "ES Write Subsequent Data Timed Out!";
                        break;
                    }
                    packageCounter++;
                }
            }

            delete buffer;
        }

        static void SimpleReadCommandSnareThread()
        {
            cClient client("ESTest");
            U8* buffer = new U8[TEST_ES_BUFFER_SIZE];

            int packageCounter = 0;

            for (U64 i = 0; i < TEST_ES_BUFFER_SIZE; i++)
            {
                buffer[i] = 0;
            }

            while ((packageCounter < NUMBER_OF_TEST_TRANSFERS) && globalMessage.empty())
            {
                U64 testSize = rand() % TEST_ES_BUFFER_SIZE;
                *((U64 *)buffer) = testSize;

                bool test = client.WriteData((U8 *)buffer, INITIAL_PACKAGE_SIZE);

                if(!test)
                {
                        globalMessage = "Snare Write Initial Package Timed Out!";
                        break;
                }

                test = client.ReadData((U8 *)buffer, testSize);

                if(!test)
                {
                        globalMessage = "Snare Read Subsequent Data Timed Out!";
                        break;
                }
                packageCounter++;
            }

            delete buffer;
        }

        TEST_METHOD(SimpleReadCommand)
        {
            std::thread serverThread(SimpleReadCommandESThread);
            //Hackish, perhaps, but we need to make sure that the server thread gets a chance to initialize the server.
            Sleep(1000);
            std::thread clientThread(SimpleReadCommandSnareThread);

            clientThread.join();
            serverThread.join();
            checkGlobalMessage();
        }

        TEST_METHOD(ProcessEvents)
        {
            PROCESS_INFORMATION ProcessInfo; //This is what we get as an [out] parameter

            STARTUPINFO StartupInfo; //This is an [in] parameter

            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            StartupInfo.cb = sizeof StartupInfo ; //Only compulsory field

            if(CreateProcess(L"..\\..\\x64\\Debug\\TestOpenEvent.exe", NULL,
            NULL,NULL,FALSE,0,NULL,
            NULL,&StartupInfo,&ProcessInfo))
            {
            }
            else
            {
                globalMessage = "Failed to start TestOpenEvent.exe";
            }

            checkGlobalMessage();

            Sleep(1000);

            SimpleReadCommandSnareThread();
            checkGlobalMessage();
        }
    };

    wchar_t ESEmulationTest::message[MAX_MESSAGE_SIZE];
    std::string ESEmulationTest::globalMessage;
}