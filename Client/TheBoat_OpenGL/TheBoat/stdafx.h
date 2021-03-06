// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 또는 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다. 
//

#pragma once
//#define _Dev

#ifdef _Dev
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#endif
#pragma comment(lib,"ws2_32")

// To Enter IP Addr
#define _IP 
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _WINSOCK_DEPRECATED_NO_WARNINGS

const float TO_RADS = 3.141592f / 180.f;

// Windows 헤더 파일:
#include <WinSock2.h>
#include <windows.h>
#include <mmsystem.h>
//#pragma comment(lib,"winmm.lib");
#include <iostream>
#include <string>
#include <mutex>
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

//
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"

//OpenGL EW
//#include "Dependencies\glew.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"


// original protocol
#include "../../../Server/TheBoat_server(OpenGL)/TheBoat_server/protocol.h"

// 교수님 확인용 protocol 복사본

// DirectX12
//#include <d3d12.h>
//#include <DirectXMath.h>
//#include <DirectXCollision.h>



// Defines
#define VIEW_ANGLE	45.f
#define SCREEN_WIDTH	1280.f
#define SCREEN_HEIGHT	720.f
#define NUM_OF_PLAYER	2
#define SIZE_MINIMAP	280.f
struct Bullet {
	bool in_use = false;
	int id;
	float x, y, z;
};

struct Item {
	int m_ItemType;
	bool m_bUse;
	float x, y, z;

};



// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.
using namespace std;