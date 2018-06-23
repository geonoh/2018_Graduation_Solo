#pragma once

class CHeightMapImage;
class Building;

struct Event {
	int id;
	int type;
	float time;
	int target;
};

class Comp {
public:
	bool operator() (const Event& left, const Event& right) {
		return (left.time > right.time);
	}
};

class ServerFramework
{
	WSADATA wsa;
	HANDLE iocp_handle;
	SOCKET listen_socket;
	SOCKADDR_IN server_addr;

	BOOL mode_selector;	// 

	Client clients[MAXIMUM_PLAYER];
	bool player_entered[4] = { 0 };
	bool player_ready[4] = { 0 };		// Player_Ready 패킷 도착하면 해당 
										// Client_ID에 맞는 배열 true
										// 모두 true가 되면 게임 시작 함수 실행
	CHeightMapImage* height_map;
	time_point<system_clock> prev_time = system_clock::now();
	float sender_time = 0;
	float item_gen_timer = 0;
	bool is_item_gen = false;
	mutex client_lock;

	// Timer전용 OverlappedExtensionSetd
	// 4는 플레이어 위치 업데이트 전용
	// 5는 충돌체크전용
	// 6은 플레이어 총알 생성
	// 7은 총알 업데이트
	OverlappedExtensionSet ol_ex[20];

	Bullet bullets[4][MAX_BULLET_SIZE] = { 0 };
	mutex bullet_lock;
	// 플레이어별 몇 번째 총알까지 발사했는지 저장하는 변수
	int bullet_counter[4] = { 0 };


	// 플레이어마다 bullet 시간을 가지고 있다. 
	float bullet_times[4];

	// Building obejct는 총 10개
	//Object* object_mother;
	Building* building[OBJECT_BUILDING];

public:
	void InitServer();
	void AcceptPlayer();
	void WorkerThread();
	void SendPacket(int cl_id, void* packet);		//
	void ProcessPacket(int cl_id, char* packet);	// 패킷 수신후 정리해서 송신
	void DisconnectPlayer(int cl_id);				// 플레이어 접속 해지

	//void TimerFunc();
	ServerFramework();
	~ServerFramework();


	// 이 함수는 ElaspsedTime을 측정하는 스레드 함수이다.
	void TimerSend(duration<float>& elapsed_time);
	// ElapsedTime을 받아와서 업데이트 하는 함수이다. 
	void Update(duration<float>& elapsed_time);

	void GameStart();
};

