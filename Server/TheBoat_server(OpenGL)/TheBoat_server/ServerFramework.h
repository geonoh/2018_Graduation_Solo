#pragma once

class CHeightMapImage;
class Building;
class Item;

struct Event {
	int id;
	int type;
	float time;
	int target;
};

//class Comp {
//public:
//	bool operator() (const Event& left, const Event& right) {
//		return (left.time > right.time);
//	}
//};

class ServerFramework
{
	WSADATA wsa;
	HANDLE iocp_handle;
	SOCKET listen_socket;
	SOCKADDR_IN server_addr;


	Client m_Clients[MAX_PLAYER];
	CHeightMapImage* height_map;
	time_point<system_clock> prev_time = system_clock::now();
	float sender_time = 0;

	float m_fBoatGenTime = 0.f;
	float m_fAmmoGenTime = 0.f;
	float m_fTimeSend = 0.f;
	bool m_bIsBoatGen = false;
	bool m_bIsAmmoGen = false;
	bool m_bGameStart = false;

	bool m_bShootStart = false;

	float m_fStartGameTime = 0.f;
	mutex m_mutexBulletLock[MAX_PLAYER];
	mutex m_mutexAmmoLock[MAX_PLAYER];
	mutex m_mutexServerLock;

	// Timer전용 OverlappedExtensionSetd
	// 4는 플레이어 위치 업데이트 전용
	// 5는 충돌체크전용
	// 6은 플레이어 총알 생성
	// 7은 총알 업데이트
	// 8은 아이템 생성 - 보트 아이템 
	// 9는 World_Time 전송
	// 10은 아이템 생성 - Ammo
	OverlappedExtensionSet ol_ex[20];

	Bullet bullets[4][MAX_AMMO] = { 0 };
	mutex bullet_lock;
	// 플레이어별 몇 번째 총알까지 발사했는지 저장하는 변수
	//int bullet_counter[4] = { 0 };


	// 플레이어마다 bullet 시간을 가지고 있다. 
	float bullet_times[4];

	// Building obejct는 총 10개
	//Object* object_mother;
	Building* building[OBJECT_BUILDING];
	Item* items[12];


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

	// ElapsedTime을 받아와서 업데이트 하는 함수이다. 
	void Update(duration<float>& elapsed_time);

	void GameStart();
};

