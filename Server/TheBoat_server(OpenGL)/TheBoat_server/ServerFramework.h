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

	Client clients[MAX_PLAYER];
	bool player_entered[4] = { 0 };
	bool player_ready[4] = { 0 };		// Player_Ready ��Ŷ �����ϸ� �ش� 
										// Client_ID�� �´� �迭 true
										// ��� true�� �Ǹ� ���� ���� �Լ� ����
	CHeightMapImage* height_map;
	time_point<system_clock> prev_time = system_clock::now();
	float sender_time = 0;

	// ��Ʈ ������ ���� �ð�
	float item_boat_gen_timer = 0.f;
	// źâ ������ ���� �ð�
	float item_ammo_gen_timer = 0.f;

	// "�ð�" ������ �ð�
	float time_sender_time = 0.f;

	// ��Ʈ ������ ����
	bool is_boat_item_gen = false;
	// źâ ������ ����
	bool is_ammo_item_gen = false;

	// ������ ���۵Ǹ� ī��Ʈ �ٿ��� �����Ѵ�. 
	bool is_game_start = false;
	// ������ ���۵� ���� �ð� ����
	float time_game_start = 0.f;

	mutex client_lock;

	// Timer���� OverlappedExtensionSetd
	// 4�� �÷��̾� ��ġ ������Ʈ ����
	// 5�� �浹üũ����
	// 6�� �÷��̾� �Ѿ� ����
	// 7�� �Ѿ� ������Ʈ
	// 8�� ������ ���� - ��Ʈ ������ 
	// 9�� World_Time ����
	// 10�� ������ ���� - Ammo
	OverlappedExtensionSet ol_ex[20];

	Bullet bullets[4][MAX_AMMO] = { 0 };
	mutex bullet_lock;
	// �÷��̾ �� ��° �Ѿ˱��� �߻��ߴ��� �����ϴ� ����
	int bullet_counter[4] = { 0 };


	// �÷��̾�� bullet �ð��� ������ �ִ�. 
	float bullet_times[4];

	// Building obejct�� �� 10��
	//Object* object_mother;
	Building* building[OBJECT_BUILDING];
	Item* items[12];


public:
	void InitServer();
	void AcceptPlayer();
	void WorkerThread();
	void SendPacket(int cl_id, void* packet);		//
	void ProcessPacket(int cl_id, char* packet);	// ��Ŷ ������ �����ؼ� �۽�
	void DisconnectPlayer(int cl_id);				// �÷��̾� ���� ����

	//void TimerFunc();
	ServerFramework();
	~ServerFramework();

	// ElapsedTime�� �޾ƿͼ� ������Ʈ �ϴ� �Լ��̴�. 
	void Update(duration<float>& elapsed_time);

	void GameStart();
};

