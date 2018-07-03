#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
//#pragma comment (lib, "winmm.lib")
#include <WinSock2.h>
#include <iostream>
#include <Windows.h>
#include <chrono>
#include <thread>

// -----------------------------------
// Server Define
#define SERVERPORT 9000
#define RUOK	1
#define RUFAIL	0

void err_quit(char *msg);
void err_display(char* msg);



using namespace std;
using namespace std::chrono;


struct Point2D {
	int x, y;
};

struct StcPacket {

};

// 해당 클라이언트에서 선언되어 값이 변경되지 않는이상 그대로 저장
// 되어있다. 
struct CtsPacket {
	UINT keyboard_click = 0x00000000;
	UINT mouse_click = 0x00000000;
	UINT player_id = 0;
	Point2D mouse_direction = { 0 };

	// 클라이언트에서 보낼 때 키보드의 클릭을 받을 때
	// OR 연산 -> 즉 하나라도 1이 있으면 TRUE를 반환한다는 뜻.
	// 중복 키 입력이 들어가는 경우 1이 미리 있을 수 있는데 이 경우도 커버 가능
	void PushArrowKeyUp() {
		UINT input_buffer = 0x10000000;
		keyboard_click = keyboard_click | input_buffer;
	}
	void PushArrowKeyDown() {
		UINT input_buffer = 0x01000000;
		keyboard_click = keyboard_click | input_buffer;
	}
	void PushArrowKeyRight() {
		UINT input_buffer = 0x00100000;
		keyboard_click = keyboard_click | input_buffer;
	}
	void PushArrowKeyLeft() {
		UINT input_buffer = 0x00010000;
		keyboard_click = keyboard_click | input_buffer;
	}
	void PushArrowKeyShift() {
		UINT input_buffer = 0x00001000;
		keyboard_click = keyboard_click | input_buffer;
	}
	void PushArrowKeySpaceBar() {
		UINT input_buffer = 0x00000100;
		keyboard_click = keyboard_click | input_buffer;
	}
	void PushArrowKeyOne() {
		UINT input_buffer = 0x00000010;
		keyboard_click = keyboard_click | input_buffer;
	}
	void PushArrowKeyTwo() {
		UINT input_buffer = 0x00000001;
		keyboard_click = keyboard_click | input_buffer;
	}


	// Release는 1을 0으로 만들어 주는 작업을 해야한다. 
	// &연산을 통해서 확인
	void ReleaseArrowKeyUp() {
		UINT input_buffer = 0x01111111;
		keyboard_click = keyboard_click & input_buffer;
	}
	void ReleaseArrowKeyDown() {
		UINT input_buffer = 0x10111111;
		keyboard_click = keyboard_click & input_buffer;
	}
	void ReleaseArrowKeyRight() {
		UINT input_buffer = 0x11011111;
		keyboard_click = keyboard_click & input_buffer;
	}
	void ReleaseArrowKeyLeft() {
		UINT input_buffer = 0x11101111;
		keyboard_click = keyboard_click & input_buffer;
	}
	void ReleaseArrowKeyShift() {
		UINT input_buffer = 0x11110111;
		keyboard_click = keyboard_click & input_buffer;
	}
	void ReleaseArrowKeySpaceBar() {
		UINT input_buffer = 0x11111011;
		keyboard_click = keyboard_click & input_buffer;
	}
	void ReleaseArrowKeyOne() {
		UINT input_buffer = 0x11111101;
		keyboard_click = keyboard_click & input_buffer;
	}
	void ReleaseArrowKeyTwo() {
		UINT input_buffer = 0x11111110;
		keyboard_click = keyboard_click & input_buffer;
	}
};

struct SocketInfo {
	OVERLAPPED overlapped;
	char buf[sizeof(CtsPacket)];
	WSABUF wsa_buffer;
	SOCKET socket;
};