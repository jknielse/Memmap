// TestOpenEvent.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>

#include "BasicTypes.h"
#include "Client.h"
#include "Server.h"
#include "Observer.h"

using namespace MemMapIPC;

#define TEST_MEMMAP_BUFFER_SIZE 160*1024
#define NUMBER_OF_TEST_TRANSFERS 500
#define TEST_ES_BUFFER_SIZE 16*1024*1024
#define INITIAL_PACKAGE_SIZE 8*1024

int _tmain(int argc, _TCHAR* argv[])
{
    cServer server("ESTest",TEST_MEMMAP_BUFFER_SIZE);
    U8* buffer = new U8[TEST_ES_BUFFER_SIZE];

    for (U64 i = 0; i < TEST_ES_BUFFER_SIZE; i++)
    {
        buffer[i] = 0;

    }

    int packageCounter = 0;

    while (packageCounter < NUMBER_OF_TEST_TRANSFERS)
    {
        if(server.NumberOfBytesAvailableToRead() >= INITIAL_PACKAGE_SIZE)
        {
            bool test = server.ReadData((U8 *)buffer, INITIAL_PACKAGE_SIZE);

            if(!test)
            {
                break;
            }

            U64 transferSize = *((U64*) buffer);

            if(transferSize > TEST_ES_BUFFER_SIZE)
            {
                break;
            }

            test = server.WriteData((U8 *)buffer, transferSize);
            if(!test)
            {
                break;
            }
            packageCounter++;
        }
    }

    delete buffer;


    return 0;
}

