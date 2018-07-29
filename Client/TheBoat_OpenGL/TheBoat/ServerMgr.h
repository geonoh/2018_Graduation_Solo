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
	glm::vec3 sc_look_vec;

	glm::vec3 collision_pos;
	float client_hp[4] = { 0 };
	int camera_id = 0;
	string server_ip;

	// 아이템 생성 부분
	glm::vec3 item_pos;
	bool is_item_gen;

	glm::vec3 building_pos[OBJECT_BUILDING];
	glm::vec3 building_extents[OBJECT_BUILDING];

	int ammo_counter = 0;


	bool s_is_collide = false;

	// 시간
	//time_point<system_clock> world_time;
	float world_time;
public:
	void IPInput();
	void IPInput(string);

	bool Initialize(HWND& hwnd);
	void ClientError();
	void ReadPacket();
	void SendPacket(int type);
	void SendPacket(int type, glm::vec3 xmvector);
	void ProcessPacket(char* ptr);
	void ErrorDisplay(const char* msg, int err_no);
	int GetAmmo();
	void DecreaseAmmo();
	int GetClientID();
	int ReturnCameraID();
	Bullet GetBullet();
	SPlayer ReturnPlayerPosStatus(int client_id);
	glm::vec3 ReturnLookVector();
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