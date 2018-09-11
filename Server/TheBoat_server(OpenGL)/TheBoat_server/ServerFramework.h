#pragma once

class CHeightMapImage;

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

	// �̰� False�� �ٲ����
	bool m_bIsBoatGen = false;	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
								//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	bool m_bIsAmmoGen = false;
	bool m_bGameStart = false;

	bool m_bShootStart = false;

	//float m_fStartGameTime = 0.f;
	mutex m_mutexBulletLock[MAX_PLAYER];
	mutex m_mutexAmmoLock[MAX_PLAYER];
	mutex m_mutexServerLock;

	// Timer���� OverlappedExtensionSetd
	// 4�� �÷��̾� ��ġ ������Ʈ ����
	// 5�� �浹üũ����
	// 6�� �÷��̾� �Ѿ� ����
	// 7�� �Ѿ� ������Ʈ
	// 8�� ������ ���� - ��Ʈ ������ 
	// 9�� World_Time ����
	// 10�� ������ ���� - Ammo
	OverlappedExtensionSet ol_ex[20];

	Bullet bullets[4][MAX_AMMO + 1] = { 0 };
	//mutex bullet_lock;

	mutex m_mutexBoatItem;
	Item m_itemBoat[4];
	Item m_itemAmmo[8];
	bool m_BoatGenedMap[4]{ false };
	int m_iDiceCounter = 0;
	int m_iDiceMapCounter = 0;
	float m_fPlayerHpUpdateTime = 0.f;
	// False = Team
	// True = Melee
	bool m_bGameMode = false;
	bool m_bWhoDie[MAX_PLAYER] = { false };
	bool m_bDieSender[MAX_PLAYER] = { false };

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

