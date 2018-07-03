#pragma once
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <Windows.h>

#define HALF_BYTE		4

using namespace std;


enum PushByte {
	Two = 0, One, SpaceBar, Shift, Left, Right, Down, Up
};


struct Point2D {
	int x, y;
};
