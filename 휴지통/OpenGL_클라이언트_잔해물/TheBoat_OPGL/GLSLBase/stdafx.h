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
#include <mutex>
//#include "../../../Server/TheBoat_server/TheBoat_server/protocol.h"
#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"
#include "../../../Server/TheBoat_server(OpenGL)/TheBoat_server/protocol.h"

#define VIEW_ANGLE	45.f
#define SCREEN_WIDTH	800
#define SCREEN_HEIGHT	600
#define NUM_OF_PLAYER	2
using namespace std;