#pragma once

// Server ���� �޾ƿ��� Player�� ���� 
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

	Bullet bullets[MAX_AMMO] = { 0 };
	int recvd_bullet_id = 0;

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
	float client_hp[4] = { 0 };
	int camera_id = 0;
	string server_ip;

	// ������ ���� �κ�
	glm::vec3 item_pos;
	bool is_item_gen;

	glm::vec3 building_pos[OBJECT_BUILDING];
	glm::vec3 building_extents[OBJECT_BUILDING];



	bool s_is_collide = false;

	// �ð�
	//time_point<system_clock> world_time;
	float world_time;

	int m_CurrentAmmo = 0;
	int m_TotalAmmo = 0;
	bool m_bNeedReloading;
	bool m_bPlayerReady[MAX_PLAYER]{ false };
	// 0 = Team
	// 1 = Melee
	bool m_bGameMode = false;
	// 0 = Red
	// 1 = Blue
	bool m_bTeam[4]{ false };
	bool m_bGameStart = false;
public:
	// 0 = Team
	// 1 = Melee
	bool GetGameMode();
	// 0 = Red
	// 1 = Blue
	bool* GetTeam();
	bool* GetPlayerReadyStatus();
	bool GetStart();
	bool GetNeedReload();
	void SetNeedReload(bool i_Need);
	void IPInput();
	void IPInput(string);

	bool Initialize(HWND& hwnd);
	void ClientError();
	void ReadPacket();
	void SendPacket(int type);
	void SendPacket(int type, glm::vec3 xmvector);
	void ProcessPacket(char* ptr);
	void ErrorDisplay(const char* msg, int err_no);
	int GetCurrentAmmo();
	int GetTotalAmmo();
	void DecreaseAmmo();
	int GetClientID();
	int ReturnCameraID();
	Bullet GetBullet();
	SPlayer ReturnPlayerPosStatus(int client_id);
	glm::vec3 ReturnLookVector(int client_id);
	glm::vec3 ReturnCollsionPosition(bool* is_collide);
	// ������ ���� �� ��ġ Return
	bool IsItemGen();
	glm::vec3 ReturnItemPosition();

	// �÷��̾� ü��
	float GetPlayerHP(int p_n);

	// 
	void ReturnBuildingPosition(glm::vec3* building_pos);
	void ReturnBuildingExtents(glm::vec3* building_pos);
};