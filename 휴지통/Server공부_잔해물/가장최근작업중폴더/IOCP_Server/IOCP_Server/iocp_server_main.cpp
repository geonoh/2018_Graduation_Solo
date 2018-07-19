// -----------------------------------------------------
// Project. The boat 
// Server Framework 
// Start From 2018. 2. 18 
// Programed by Gunny
// -----------------------------------------------------


#include "stdafx.h"
#include "ServerFramework.h"

ServerFramework server_framework;
UINT player_counter = 0;
void InitServer();
void PlayingSession();
BOOL LoadAndSendInit();


int main() {
	InitServer();
	while (true) {
		// for문으로 대체 가능?
		player_counter++;
		if (player_counter <= 4) {
			if (server_framework.AcceptClient(player_counter) == RUOK) {
				cout << player_counter << " player enterd" << endl;
			}
		}
		if (player_counter == 4) {
			break;
		}
	}
	cout << "4명의 플레이어 모두 서버 접속 완료" << endl;
	if (!LoadAndSendInit()) {
		cout << "LoadAndSendInit() 에러" << endl;
		return 0;
	}
	while (true) {
		PlayingSession();
	}

	cout << "Server Quit" << endl;
	return 0;
}


void InitServer() {
	server_framework.Initialize();
}

void PlayingSession() {
	// 1. 업데이트 
	// 2. 충돌체크 
	// 3. 서버->클라이언트 데이터 송신
	server_framework.Update();

	server_framework.CollideCheck();
}

BOOL LoadAndSendInit() {
	// 여기서 플레이어의 위치
	// 게임 타이머 
	// 

	return TRUE;
}