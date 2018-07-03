#pragma once
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <iostream>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000

enum ClientID {
	client_id_1 = 1,
	client_id_2,
	client_id_3,
	client_id_4
};



struct Point2D {
	float x, y;
};

void err_quit(const char *msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	//MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char *msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}


// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags) {
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}
	return (len - left);
}


// ------------------------------------------------------------------------------

// 클라이언트에서 서버로 보내는 패킷
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