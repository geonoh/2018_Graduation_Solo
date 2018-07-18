#pragma once
#pragma comment(lib,"ws2_32")
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <WinSock2.h>
#include <iostream>
#include <string>
#include <thread>
//#include "../../../Server/TheBoat_server/TheBoat_server/protocol.h"
#include "protocol.h"
#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"


#define VIEW_ANGLE	45.f

using namespace std;