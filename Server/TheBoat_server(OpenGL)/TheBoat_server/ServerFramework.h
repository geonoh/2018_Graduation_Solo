#pragma once

class CHeightMapImage;

class ServerFramework
{
	WSADATA wsa;
	HANDLE iocp_handle;
	SOCKET listen_socket;
	SOCKADDR_IN server_addr;


	Client clients[MAX_PLAYER];
	CHeightMapImage* height_map;
	time_point<system_clock> prev_time = system_clock::now();
	float sender_time = 0;

	float time_boat = 0.f;
	float time_ammo = 0.f;
	float time_send = 0.f;

	bool is_boat_item_gen = false;	

	bool ammo_gen = false;
	bool game_start = false;

	bool is_shoot_start = false;

	mutex lock_bullet[MAX_PLAYER];
	mutex lock_ammo[MAX_PLAYER];
	mutex lock_server;

	OverlappedExtensionSet ol_ex[20];

	Bullet bullets[4][MAX_AMMO + 1] = { 0 };

	mutex lock_boat_item;
	Item item_boat[4];
	Item item_ammo[8];
	bool map_boat_gen[4]{ false };
	int count_dice = 0;
	int count_map_dice = 0;
	float time_player_hp_update = 0.f;
	/*
	False = Team
	True = Melee
	*/
	bool game_mode = false;
	bool is_die_sended[MAX_PLAYER] = { false };

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

