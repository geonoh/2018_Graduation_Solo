#pragma once
//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"ws2_32")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include "protocol.h"
#include "defines.h"


using namespace std;