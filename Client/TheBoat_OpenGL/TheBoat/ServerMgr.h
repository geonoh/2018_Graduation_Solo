#pragma once

// Server 에서 받아오는 Player의 정보 
struct SPlayer {
	glm::vec3 pos;
	int player_status;
};


class ServerMgr
{
	SOCKET sock;
	SOCKADDR_IN server_addr;
	HWND async_handle;
	WSABUF send_wsabuf;
	WSABUF recv_wsabuf;
	int clients_id = 0;


	bool first_set_id = true;

	char send_buffer[CLIENT_BUF_SIZE] = { 0 };
	//char send_buffer_vector[CLIENT_BUF_SIZE] = { 0 };
	char recv_buffer[CLIENT_BUF_SIZE] = { 0 };

	char packet_buffer[CLIENT_BUF_SIZE] = { 0 };
	DWORD in_packet_size = 0;
	DWORD saved_packet_size = 0;

	SPlayer sc_vec_buff[4];
	glm::vec3 m_v3PlayerLookVector[MAX_PLAYER];

	glm::vec3 collision_pos;
	int camera_id = 0;
	string server_ip;

	// 아이템 생성 부분
	glm::vec3 item_pos;
	bool is_item_gen;

	glm::vec3 building_pos[OBJECT_BUILDING];
	glm::vec3 building_extents[OBJECT_BUILDING];



	bool s_is_collide = false;
	Bullet m_Bullets[MAX_PLAYER][MAX_AMMO + 1] = { 0 };
	// 시간
	//time_point<system_clock> world_time;
	float m_fWorldTime;

	int m_CurrentAmmo = 0;
	int m_TotalAmmo = 0;
	bool m_bNeedReloading;
	bool m_bPlayerReady[MAX_PLAYER]{ false };
	// 0 = Team
	// 1 = Melee
	bool m_bGameMode = false;
	// 0 = Red
	// 1 = Blue
	//bool m_bTeam[4]{ false };
	bool m_bGameStart = false;

	Item m_itemBoat[4];
	Item m_itemAmmo[8];
	float client_hp[MAX_PLAYER] = { 0.f };

	bool m_bPlayerBoatParts[4] = { false };
	// 낮 -> false(비)
	// 밤 -> true (눈)
	bool m_bWeather = false;
	Team m_TeamPlayer[MAX_PLAYER];
	bool m_bPlayerDie[MAX_PLAYER] = { false };
	char m_cResult = 0;
	float m_fStamina[MAX_PLAYER];
public:
	float GetStamina(int iPlayerID);
	char GetResult();
	bool GetPlayerDie(int iPlayerID);
	Team GetTeam(int iPlayerID);
	bool GetWeather();
	bool GetPlayerHaveParts(int iPartsType);
	Item GetBoatItem(int iItemNumber);
	Item GetAmmoItem(int iItemNumber);
	mutex m_mutexBulletLock[MAX_PLAYER];
	int GetCurrentAmmo();
	int GetTotalAmmo();

	// 0 = Team
	// 1 = Melee
	bool GetGameMode();
	// 0 = Red
	// 1 = Blue
	//bool* GetTeam();
	bool* GetPlayerReadyStatus();
	float GetTime();
	bool GetStart();
	bool GetNeedReload();
	void SetNeedReload(bool i_Need);
	void IPInput();
	void IPInput(string);
	Bullet GetBullet(int i_Player, int i_Bullet);

	bool Initialize(HWND& hwnd);
	void ClientError();
	void ReadPacket();
	void SendPacket(int type);
	void SendPacket(int type, glm::vec3 xmvector);
	void ProcessPacket(char* ptr);
	void ErrorDisplay(const char* msg, int err_no);
	void DecreaseAmmo();
	int GetClientID();
	int ReturnCameraID();
	//Bullet GetBullet();
	SPlayer ReturnPlayerPosStatus(int client_id);
	glm::vec3 ReturnLookVector(int client_id);
	glm::vec3 ReturnCollsionPosition(bool* is_collide);
	// 아이템 생성 후 위치 Return
	bool IsItemGen();
	glm::vec3 ReturnItemPosition();

	// 플레이어 체력
	float GetPlayerHP(int p_n);

	// 
	void ReturnBuildingPosition(glm::vec3* building_pos);
	void ReturnBuildingExtents(glm::vec3* building_pos);
};