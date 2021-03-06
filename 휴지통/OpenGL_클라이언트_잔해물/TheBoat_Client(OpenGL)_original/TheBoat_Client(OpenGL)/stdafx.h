#pragma once
#define _DEV

#ifdef _DEV
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#endif
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "glew32.lib")
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "targetver.h"

// Windows 헤더 파일:
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <string>

// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include "../../../../Server/TheBoat_server(OpenGL)/TheBoat_server/protocol.h"
// Defines
#define VIEW_ANGLE	45.f
#define SCREEN_WIDTH	800
#define SCREEN_HEIGHT	600
#define NUM_OF_PLAYER	2



// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.
using namespace std;