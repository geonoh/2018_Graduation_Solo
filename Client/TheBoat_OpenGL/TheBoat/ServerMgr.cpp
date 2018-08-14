#include "stdafx.h"
#include "ServerMgr.h"


void ServerMgr::ErrorDisplay(const char* msg, int err_no) {
	_wsetlocale(LC_ALL, L"korean");
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("%s", msg);
	wprintf(L"%s\n", lpMsgBuf);
	LocalFree(lpMsgBuf);
}
void ServerMgr::IPInput() {
	while (true) {
		cout << "서버 아이피 입력 : ";
		cin >> server_ip;
		break;
	}
}
void ServerMgr::IPInput(string i_server_ip) {
	server_ip = i_server_ip;
}

bool ServerMgr::Initialize(HWND& hwnd) {
	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	int opt_val = TRUE;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt_val, sizeof(opt_val));

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	// 아이피
	ServerAddr.sin_addr.s_addr = inet_addr(server_ip.c_str());
	//ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//ServerAddr.sin_addr.s_addr = inet_addr("110.5.195.3");

	int retval = WSAConnect(sock, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);
	if (retval == SOCKET_ERROR) {
		printf("---------------------------------\n");
		printf("- Connect Err\n");
		printf("---------------------------------\n");
		return false;
	}
	async_handle = hwnd;
	WSAAsyncSelect(sock, async_handle, WM_SOCKET, FD_CONNECT | FD_CLOSE | FD_READ);

	send_wsabuf.buf = send_buffer;
	send_wsabuf.len = CLIENT_BUF_SIZE;
	recv_wsabuf.buf = recv_buffer;
	recv_wsabuf.len = CLIENT_BUF_SIZE;

	// 총알 갯수 초기화 
	m_CurrentAmmo = 0;
	m_TotalAmmo = 0;
	return true;
}

int ServerMgr::GetCurrentAmmo() {
	return m_CurrentAmmo;
}

int ServerMgr::GetTotalAmmo() {
	return m_TotalAmmo;
}

void ServerMgr::DecreaseAmmo() {
	m_CurrentAmmo -= 1;
}

void ServerMgr::ReadPacket() {
	DWORD io_bytes, io_flag = 0;

	int retval = WSARecv(sock, &recv_wsabuf, 1, &io_bytes, &io_flag, NULL, NULL);
	if (retval == 1) {
		int err_code = WSAGetLastError();
		ErrorDisplay("[WSARecv] : 에러 ", err_code);
	}
	BYTE* ptr = reinterpret_cast<BYTE*>(recv_buffer);

	while (io_bytes != 0) {
		if (in_packet_size == 0)
			in_packet_size = ptr[0];
		if (io_bytes + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_bytes -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_bytes);
			saved_packet_size += io_bytes;
			io_bytes = 0;
		}

	}
}
//Bullet ServerMgr::GetBullet() {
//	return bullets[recvd_bullet_id];
//}

Bullet ServerMgr::GetBullet(int i_Player, int i_Bullet) {
	return m_Bullets[i_Player][i_Bullet];
}



int ServerMgr::GetClientID() {
	return clients_id;
}

float ServerMgr::GetStamina(int iPlayerID) {
	return m_fStamina[iPlayerID];
}

void ServerMgr::ProcessPacket(char* ptr) {
	static bool first_time = true;
	switch (ptr[1]) {
	case SC_ENTER_PLAYER: {
		SC_PACKET_ENTER_PLAYER * packets = reinterpret_cast<SC_PACKET_ENTER_PLAYER*>(ptr);
		if (first_set_id) {
			clients_id = packets->id;
			camera_id = packets->id;
			m_CurrentAmmo = packets->m_CurrentAmmo;
			m_TotalAmmo = packets->m_TotalAmmo;
			m_fStamina[clients_id] = packets->m_fStamina;
			first_set_id = false;
		}
		sc_vec_buff[packets->id].pos.x = packets->x;
		sc_vec_buff[packets->id].pos.y = packets->y;
		sc_vec_buff[packets->id].pos.z = packets->z;
		client_hp[packets->id] = packets->hp;
		m_fStamina[clients_id] = packets->m_fStamina;
		printf("[SC_ENTER_PLAYER] : %d 플레이어 입장\n", packets->id);

		break;
	}
	case SC_BUILDING_GEN: {
		SC_PACKET_ENTER_PLAYER* packets = reinterpret_cast<SC_PACKET_ENTER_PLAYER*>(ptr);
		building_pos[packets->id].x = packets->x;
		building_pos[packets->id].y = packets->y;
		building_pos[packets->id].z = packets->z;

		building_extents[packets->id].x = packets->size_x;
		building_extents[packets->id].y = packets->size_y;
		building_extents[packets->id].z = packets->size_z;
		//printf("[%d] 빌딩 [%f, %f, %f] 크기 : [%f, %f, %f] \n", packets->id,
		//	building_pos[packets->id].x,
		//	building_pos[packets->id].y,
		//	building_pos[packets->id].z,
		//	building_extents[packets->id].x,
		//	building_extents[packets->id].y,
		//	building_extents[packets->id].z);
		break;
	}

	case SC_POS: {
		SC_PACKET_POS* packets = reinterpret_cast<SC_PACKET_POS*>(ptr);
		clients_id = packets->id;
		sc_vec_buff[packets->id].pos.x = packets->x;
		sc_vec_buff[packets->id].pos.y = packets->y;
		sc_vec_buff[packets->id].pos.z = packets->z;
		m_fStamina[clients_id] = packets->m_fStamina;
		break;
	}
	case SC_STAMINA: {
		SC_PACKET_STAMINA* packets = reinterpret_cast<SC_PACKET_STAMINA*>(ptr);
		m_fStamina[packets->m_cID] = packets->m_fStamina;
		break;
	}
	case SC_PLAYER_LOOKVEC: {
		SC_PACKET_LOOCVEC* packets = reinterpret_cast<SC_PACKET_LOOCVEC*>(ptr);
		clients_id = packets->id;
		m_v3PlayerLookVector[clients_id] = packets->look_vec;
		break;
	}
	case SC_BULLET_POS: {
		SC_PACKET_BULLET* packets = reinterpret_cast<SC_PACKET_BULLET*>(ptr);
		clients_id = packets->id;
		m_Bullets[clients_id][packets->bullet_id].id = packets->bullet_id;
		m_Bullets[clients_id][packets->bullet_id].x = packets->x;
		m_Bullets[clients_id][packets->bullet_id].y = packets->y;
		m_Bullets[clients_id][packets->bullet_id].z = packets->z;
		m_Bullets[clients_id][packets->bullet_id].in_use = packets->m_bInUse;
		break;
	}
	case SC_COLLSION_TB: {
		SC_PACKET_COLLSION_TB* packets = reinterpret_cast<SC_PACKET_COLLSION_TB*>(ptr);
		m_Bullets[packets->m_cPlayerID][packets->m_cBulletID].in_use = false;
		break;
	}
	case SC_ITEM_GEN: {
		// 아이템 생성
		SC_PACKET_ITEM_GEN* packets = reinterpret_cast<SC_PACKET_ITEM_GEN*>(ptr);
		item_pos.x = packets->x;
		item_pos.y = packets->y;
		item_pos.z = packets->z;
		printf("아템 생성\n");
		is_item_gen = true;
		break;
	}
	case SC_FULLY_AMMO: {

		SC_PACKET_AMMO_O* packets = reinterpret_cast<SC_PACKET_AMMO_O*>(ptr);
		m_CurrentAmmo = packets->ammo;
		m_TotalAmmo = packets->m_cTotalAmmo;

		printf("재장전 완료\n");
		break;
	}
	case SC_AMMO: {
		SC_PACKET_AMMO* packets = reinterpret_cast<SC_PACKET_AMMO*>(ptr);
		m_CurrentAmmo = packets->m_CurrentAmmo;
		m_TotalAmmo = packets->m_TotalAmmo;
		break;
	}
	case SC_OUT_OF_AMMO: {
		printf("총알 다씀\n");
		m_bNeedReloading = true;
		break;
	}
	case SC_WORLD_TIME: {
		SC_PACKET_TIME* packets = reinterpret_cast<SC_PACKET_TIME*>(ptr);
		m_fWorldTime = packets->world_time;
		printf("시간은 : %f\n", m_fWorldTime);
		break;
	}
	case SC_PLAYER_READY: {
		SC_PACKET_READY* packets = reinterpret_cast<SC_PACKET_READY*>(ptr);
		m_bPlayerReady[packets->m_cPlayerNumber] = true;
		break;
	}
	case SC_PLAYER_READY_CANCLE: {
		SC_PACKET_READY* packets = reinterpret_cast<SC_PACKET_READY*>(ptr);
		m_bPlayerReady[packets->m_cPlayerNumber] = false;
		break;
	}
	case SC_MODE_TEAM: {
		m_bGameMode = false;
		break;
	}
	case SC_MODE_MELEE: {
		m_bGameMode = true;
		break;
	}
	case SC_TEAM_RED: {
		CS_PACKET_TEAM_SELECT* packets = reinterpret_cast<CS_PACKET_TEAM_SELECT*>(ptr);
		m_TeamPlayer[packets->m_cID] = e_TeamRed;
		break;
	}
	case SC_TEAM_BLUE: {
		CS_PACKET_TEAM_SELECT* packets = reinterpret_cast<CS_PACKET_TEAM_SELECT*>(ptr);
		m_TeamPlayer[packets->m_cID] = e_TeamBlue;
		break;
	}
	case SC_GAME_START: {
		SC_PACKET_START* packets = reinterpret_cast<SC_PACKET_START*>(ptr);
		for (int i = 0; i < MAX_PLAYER; ++i) {
			m_TeamPlayer[i] = packets->m_bPlayerTeam[i];
			printf("%dPlayer Team : %d\n", i, m_TeamPlayer[i]);
		}
		m_bGameStart = true;
		m_cResult = 0;
		break;
	}
	case SC_BOAT_ITEM_GEN: {
		SC_PACKET_ITEM_GEN * packets = reinterpret_cast<SC_PACKET_ITEM_GEN*>(ptr);
		int iItemType = packets->item_type;
		m_itemBoat[iItemType].m_ItemType = iItemType;
		m_itemBoat[iItemType].m_bUse = true;
		m_itemBoat[iItemType].x = packets->x;
		m_itemBoat[iItemType].y = packets->y;
		m_itemBoat[iItemType].z = packets->z;

		printf("아이템 도착 Type %d, [%f, %f, %f] \n", iItemType,
			m_itemBoat[iItemType].x,
			m_itemBoat[iItemType].y,
			m_itemBoat[iItemType].z);
		break;
	}
	case SC_PLAYER_HP_UPDATE: {
		SC_PACKET_PLAYER_HP_UPDATE * packets = reinterpret_cast<SC_PACKET_PLAYER_HP_UPDATE*>(ptr);
		printf("%d Player HP : %d \n", packets->m_cPlayerID, packets->m_fHp);
		client_hp[packets->m_cPlayerID] = packets->m_fHp;
		break;
	}
	case SC_AMMO_ITEM_GEN: {
		SC_PACKET_ITEM_GEN * packets = reinterpret_cast<SC_PACKET_ITEM_GEN*>(ptr);
		// packets->item_type
		m_itemAmmo[packets->m_cItemID].m_bUse = true;
		m_itemAmmo[packets->m_cItemID].x = packets->x;
		m_itemAmmo[packets->m_cItemID].y = packets->y;
		m_itemAmmo[packets->m_cItemID].z = packets->z;
		break;
	}
	case SC_PLAYER_GET_ITEM: {
		SC_PACKET_GET_ITEM * packets = reinterpret_cast<SC_PACKET_GET_ITEM*>(ptr);
		if (packets->m_cItemType < 4) {
			if (packets->m_cGetterID == clients_id) {
				m_bPlayerBoatParts[packets->m_cItemType] = true;
			}
			m_itemBoat[packets->m_cItemType].m_bUse = false;
		}
		else if (packets->m_cItemType == 4) {
			if (packets->m_cGetterID == clients_id) {
				m_TotalAmmo = 90;
			}
			m_itemAmmo[packets->m_cAmmoItemID].m_bUse = false;
			printf("%d 아이템 use false로 바꿨다. 분명\n", packets->m_cAmmoItemID);
		}
		break;
	}
	case SC_WEATHER_CHANGE: {
		SC_PACKET_WEATHER * packets = reinterpret_cast<SC_PACKET_WEATHER*>(ptr);
		m_bWeather = true;
		break;
	}
	case SC_HIT: {
		SC_PACKET_HIT * packets = reinterpret_cast<SC_PACKET_HIT*>(ptr);
		m_Bullets[packets->m_cShooterID][packets->m_cBulletNumber].in_use = false;
		client_hp[packets->m_cHitID] = packets->m_fHp;
		printf("SC_HIT, 때린이 %d 맞은이 %d \n", packets->m_cShooterID, packets->m_cHitID);
		break;
	}
	case SC_PLAYER_DIE: {
		SC_PACKET_DIE * packets = reinterpret_cast<SC_PACKET_DIE*>(ptr);
		m_bPlayerDie[packets->m_cDiePlayer] = true;
		break;
	}
	case SC_RESULT: {
		SC_PACKET_RESULT * packets = reinterpret_cast<SC_PACKET_RESULT*>(ptr);
		m_cResult = packets->m_cVictoryTeam;
		m_bGameStart = false;
		break;
	}
	case SC_ENTER_LOBBY: {
		m_cResult = 0;
		m_bWeather = false;
		m_bGameStart = false;
		m_bGameMode = 0;
		m_TotalAmmo = 90;
		m_CurrentAmmo = 30;
		for (int i = 0; i < 8; ++i) {
			m_itemAmmo[i].m_bUse = false;
		}
		for (int i = 0; i < MAX_PLAYER; ++i) {
			m_bPlayerReady[i] = false;
			m_TeamPlayer[i] = e_NoTeam;
			m_bPlayerDie[i] = false;
			client_hp[i] = 100.f;
			for (int j = 1; j < MAX_AMMO + 1; ++j) {
				m_Bullets[i][j].in_use = false;
			}
		}
		// 아이템
		for (int i = 0; i < 4; ++i) {
			m_itemBoat[i].m_bUse = false;
			m_bPlayerBoatParts[i] = false;
		}
		break;
	}
	}
}

Item ServerMgr::GetAmmoItem(int iItemNumber) {
	return m_itemAmmo[iItemNumber];
}

char ServerMgr::GetResult() {
	return m_cResult;
}

bool ServerMgr::GetPlayerDie(int iPlayerID) {
	return m_bPlayerDie[iPlayerID];
}

bool ServerMgr::GetWeather() {
	return m_bWeather;
}

bool ServerMgr::GetPlayerHaveParts(int iPartsType) {
	return m_bPlayerBoatParts[iPartsType];
}

Item ServerMgr::GetBoatItem(int iItemNumber) {
	return m_itemBoat[iItemNumber];
}

float ServerMgr::GetTime() {
	return m_fWorldTime;
}

bool ServerMgr::GetStart() {
	return m_bGameStart;
}

Team ServerMgr::GetTeam(int iPlayerID) {
	return m_TeamPlayer[iPlayerID];
}

bool ServerMgr::GetGameMode() {
	return m_bGameMode;
}

bool* ServerMgr::GetPlayerReadyStatus() {
	return m_bPlayerReady;
}

bool ServerMgr::GetNeedReload() {
	return false;
}
void ServerMgr::SetNeedReload(bool i_Need) {

}

float ServerMgr::GetPlayerHP(int p_n) {
	return client_hp[p_n];

}
bool ServerMgr::IsItemGen() {
	return is_item_gen;
}

void ServerMgr::ReturnBuildingPosition(glm::vec3* input_building_pos) {
	for (int i = 0; i < OBJECT_BUILDING; ++i) {
		input_building_pos[i].x = building_pos[i].x;
		input_building_pos[i].y = building_pos[i].y;
		input_building_pos[i].z = building_pos[i].z;
	}
}

void ServerMgr::ReturnBuildingExtents(glm::vec3* input_building_extents) {
	for (int i = 0; i < OBJECT_BUILDING; ++i) {
		input_building_extents[i].x = building_extents[i].x;
		input_building_extents[i].y = building_extents[i].y;
		input_building_extents[i].z = building_extents[i].z;
	}
}


glm::vec3 ServerMgr::ReturnItemPosition() {
	is_item_gen = false;
	return item_pos;
}

glm::vec3 ServerMgr::ReturnCollsionPosition(bool* is_collide) {
	*is_collide = s_is_collide;
	s_is_collide = false;
	return collision_pos;
}

void ServerMgr::SendPacket(int type) {
	CS_PACKET_KEYUP* packet_buffer = reinterpret_cast<CS_PACKET_KEYUP*>(send_buffer);
	packet_buffer->size = sizeof(CS_PACKET_KEYUP);
	send_wsabuf.len = sizeof(CS_PACKET_KEYUP);
	int retval = 0;
	DWORD iobytes;
	switch (type) {
	case CS_KEY_PRESS_UP:
		packet_buffer->type = CS_KEY_PRESS_UP;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_DOWN:
		packet_buffer->type = CS_KEY_PRESS_DOWN;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_RIGHT:
		packet_buffer->type = CS_KEY_PRESS_RIGHT;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_LEFT:
		packet_buffer->type = CS_KEY_PRESS_LEFT;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_KEY_PRESS_SHIFT:
		packet_buffer->type = CS_KEY_PRESS_SHIFT;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_SPACE:
		packet_buffer->type = CS_KEY_PRESS_SPACE;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_1:
		packet_buffer->type = CS_KEY_PRESS_1;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_2:
		packet_buffer->type = CS_KEY_PRESS_2;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;


	case CS_KEY_RELEASE_UP:
		packet_buffer->type = CS_KEY_RELEASE_UP;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_DOWN:
		packet_buffer->type = CS_KEY_RELEASE_DOWN;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_RIGHT:
		packet_buffer->type = CS_KEY_RELEASE_RIGHT;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_LEFT:
		packet_buffer->type = CS_KEY_RELEASE_LEFT;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_KEY_RELEASE_SHIFT:
		packet_buffer->type = CS_KEY_RELEASE_SHIFT;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_SPACE:
		packet_buffer->type = CS_KEY_RELEASE_SPACE;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_1:
		packet_buffer->type = CS_KEY_RELEASE_1;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_2:
		packet_buffer->type = CS_KEY_RELEASE_2;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;


	case CS_LEFT_BUTTON_DOWN:
		packet_buffer->type = CS_LEFT_BUTTON_DOWN;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_RIGHT_BUTTON_DOWN:
		packet_buffer->type = CS_RIGHT_BUTTON_DOWN;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_LEFT_BUTTON_UP:
		packet_buffer->type = CS_LEFT_BUTTON_UP;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_RIGHT_BUTTON_UP:
		packet_buffer->type = CS_RIGHT_BUTTON_UP;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_MOUSE_MOVE:
		packet_buffer->type = CS_MOUSE_MOVE;
		// 여기에 추가적으로 player의 look 벡터를 같이 해서 보내줘야한다. 
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
	case CS_PLAYER_READY:
		packet_buffer->type = CS_PLAYER_READY;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_PLAYER_READY_CANCLE:
		packet_buffer->type = CS_PLAYER_READY_CANCLE;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_RELOAD:
		packet_buffer->type = CS_RELOAD;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_TEAM_RED:
		packet_buffer->type = CS_TEAM_RED;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_TEAM_BLUE:
		packet_buffer->type = CS_TEAM_BLUE;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_MODE_TEAM:
		packet_buffer->type = CS_MODE_TEAM;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_MODE_MELEE:
		packet_buffer->type = CS_MODE_MELEE;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_DEBUG_TIME:
		packet_buffer->type = CS_DEBUG_TIME;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_ASSENBLE_PARTS:
		packet_buffer->type = CS_ASSENBLE_PARTS;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_RESTART_GAME:
		packet_buffer->type = CS_RESTART_GAME;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_ENTER_LOBBY:
		packet_buffer->type = CS_ENTER_LOBBY;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	}
	if (retval == 1) {
		int error_code = WSAGetLastError();
		ErrorDisplay("[WSASend] 에러 : ", error_code);
	}

}
void ServerMgr::SendPacket(int type, glm::vec3 v3LookVec) {
	CS_PACKET_KEYUP* packet_buffer = reinterpret_cast<CS_PACKET_KEYUP*>(send_buffer);
	packet_buffer->size = sizeof(CS_PACKET_KEYUP);
	send_wsabuf.len = sizeof(CS_PACKET_KEYUP);
	int retval = 0;
	DWORD iobytes;
	switch (type) {
	case CS_KEY_PRESS_UP:
		packet_buffer->type = CS_KEY_PRESS_UP;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_DOWN:
		packet_buffer->type = CS_KEY_PRESS_DOWN;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_RIGHT:
		packet_buffer->type = CS_KEY_PRESS_RIGHT;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_LEFT:
		packet_buffer->type = CS_KEY_PRESS_LEFT;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_KEY_PRESS_SHIFT:
		packet_buffer->type = CS_KEY_PRESS_SHIFT;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_SPACE:
		packet_buffer->type = CS_KEY_PRESS_SPACE;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_1:
		packet_buffer->type = CS_KEY_PRESS_1;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_PRESS_2:
		packet_buffer->type = CS_KEY_PRESS_2;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;


	case CS_KEY_RELEASE_UP:
		packet_buffer->type = CS_KEY_RELEASE_UP;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_DOWN:
		packet_buffer->type = CS_KEY_RELEASE_DOWN;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_RIGHT:
		packet_buffer->type = CS_KEY_RELEASE_RIGHT;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_LEFT:
		packet_buffer->type = CS_KEY_RELEASE_LEFT;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_KEY_RELEASE_SHIFT:
		packet_buffer->type = CS_KEY_RELEASE_SHIFT;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_SPACE:
		packet_buffer->type = CS_KEY_RELEASE_SPACE;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_1:
		packet_buffer->type = CS_KEY_RELEASE_1;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_KEY_RELEASE_2:
		packet_buffer->type = CS_KEY_RELEASE_2;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;


	case CS_LEFT_BUTTON_DOWN:
		packet_buffer->type = CS_LEFT_BUTTON_DOWN;
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_RIGHT_BUTTON_DOWN:
		packet_buffer->type = CS_RIGHT_BUTTON_DOWN;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_LEFT_BUTTON_UP:
		packet_buffer->type = CS_LEFT_BUTTON_UP;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;
	case CS_RIGHT_BUTTON_UP:
		packet_buffer->type = CS_RIGHT_BUTTON_UP;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
		break;

	case CS_MOUSE_MOVE:
		packet_buffer->type = CS_MOUSE_MOVE;
		// 여기에 추가적으로 player의 look 벡터를 같이 해서 보내줘야한다.
		packet_buffer->look_vec = v3LookVec;
		retval = WSASend(sock, &send_wsabuf, 1, &iobytes, 0, NULL, NULL);
	}
	if (retval == 1) {
		int error_code = WSAGetLastError();
		ErrorDisplay("[WSASend] 에러 : ", error_code);
	}

}

void ServerMgr::ClientError() {
	exit(-1);
}

SPlayer ServerMgr::ReturnPlayerPosStatus(int client_id) {
	return sc_vec_buff[client_id];
}

glm::vec3 ServerMgr::ReturnLookVector(int client_id) {
	return m_v3PlayerLookVector[client_id];
}
int ServerMgr::ReturnCameraID() {
	return camera_id;
}