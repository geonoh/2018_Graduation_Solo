#include "stdafx.h"
#include "ServerFramework.h"
#include "CHeightMapImage.h"

void ErrorDisplay(const char* msg, int err_no) {
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << L"에러 " << lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

ServerFramework::ServerFramework()
{
}

ServerFramework::~ServerFramework()
{
	delete height_map;
}

void ServerFramework::InitServer() {
	wcout.imbue(locale("korean"));

	srand(unsigned(time(NULL)));
	int retval = 0;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		printf("WSAStartup() 에러\n");

	iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp_handle == NULL)
		printf("최초: CreateIoCompletionPort() 에러\n");

	// 비동기 방식의 Listen 소켓 생성
	listen_socket = WSASocketW(AF_INET, SOCK_STREAM,
		IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt_val = TRUE;
	setsockopt(listen_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&opt_val, sizeof(opt_val));
	if (listen_socket == INVALID_SOCKET)
		printf("listen_socket 생성 오류\n");

	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_PORT);         // 9000번 포트
	retval = ::bind(listen_socket, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR)
		printf("bind 에러\n");

	retval = listen(listen_socket, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		printf("listen 에러\n");

	XMFLOAT3 xmf3Scale(1.f, 1.f, 1.f);
	LPCTSTR file_name = _T("height_map.raw");
	height_map = new CHeightMapImage(file_name, 513, 513, xmf3Scale);

	for (int i = 0; i < MAX_PLAYER; ++i) {
		m_Clients[i].x = 0.f;
		m_Clients[i].z = 0.f;
		m_Clients[i].y = height_map->GetHeight(m_Clients[i].x + DX12_TO_OPGL, m_Clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
		m_Clients[i].m_fhp = 100.f;
		m_Clients[i].m_fStamina = 100.f;
		m_Clients[i].m_CurrentAmmo = 30;
		m_Clients[i].m_TotalAmmo = 90;
		for (int j = 0; j < 4; ++j) {
			m_Clients[i].m_bPlayerBoatParts[j] = false;
		}
	}
#ifdef _Dev
	printf("---------------------------------\n");
	printf("- 개발모드\n");
	printf("---------------------------------\n");
	m_bIsBoatGen = true;
	m_bGameStart = true;
	m_bIsAmmoGen = true;
	m_fBoatGenTime = 0.f;
	for (int i = 0; i < MAX_PLAYER; ++i) {
		m_Clients[i].x = 158.f;
		m_Clients[i].y = 45.f;
		m_Clients[i].z = -183.f;
		m_Clients[i].m_fStamina = 100.f;
	}
#endif


}

void ServerFramework::AcceptPlayer() {
	SOCKADDR_IN c_addr;
	ZeroMemory(&c_addr, sizeof(SOCKADDR_IN));
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(SERVER_PORT);
	c_addr.sin_addr.s_addr = INADDR_ANY;
	int addr_len = sizeof(SOCKADDR_IN);

	int new_key = -1;
	auto client_socket = WSAAccept(listen_socket, reinterpret_cast<SOCKADDR*>(&c_addr), &addr_len, NULL, NULL);
	// Nagle알고리즘
	int opt_val = TRUE;
	setsockopt(listen_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&opt_val, sizeof(opt_val));
	//
	printf("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port));

	int client_id = -1;
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (m_Clients[i].m_bUsing == false) {
			client_id = i;
			break;
		}
	}
	if (client_id == -1) {
		printf("최대 유저 초과\n");
	}
	printf("[%d] 플레이어 입장\n", client_id);
	m_Clients[client_id].s = client_socket;

	m_Clients[client_id].m_bReady = false;
	m_Clients[client_id].m_bRunning = false;
	ZeroMemory(&m_Clients[client_id].overlapped_ex.wsa_over, sizeof(WSAOVERLAPPED));
	//m_Clients[client_id].overlapped_ex.is_recv = true;
	m_Clients[client_id].overlapped_ex.evt_type = EVT_RECV_PACKET;
	m_Clients[client_id].overlapped_ex.wsabuf.buf = m_Clients[client_id].overlapped_ex.io_buffer;
	m_Clients[client_id].overlapped_ex.wsabuf.len = sizeof(m_Clients[client_id].overlapped_ex.io_buffer);
	m_Clients[client_id].m_bPacketSize = 0;
	m_Clients[client_id].m_bPrevPacketSize = 0;



	//m_Clients[client_id].m_Team = e_TeamRed;
	m_Clients[client_id].m_Team = e_NoTeam;


	// 플레이어 입장 표시
	m_Clients[client_id].m_bUsing = true;

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(client_socket),
		iocp_handle, client_id, 0);
	unsigned long flag = 0;
	WSARecv(client_socket, &m_Clients[client_id].overlapped_ex.wsabuf, 1, NULL,
		&flag, &m_Clients[client_id].overlapped_ex.wsa_over, NULL);

	// 플레이어 입장했다고 패킷 보내줘야함.
	// 이 정보에는 플레이어의 초기 위치정보도 포함되어야 한다. 
	SC_PACKET_ENTER_PLAYER packet;
	packet.id = client_id;
	packet.size = sizeof(SC_PACKET_ENTER_PLAYER);
	packet.type = SC_ENTER_PLAYER;
	packet.hp = m_Clients[client_id].m_fhp;
	packet.m_fStamina = m_Clients[client_id].m_fStamina;
	packet.x = m_Clients[client_id].x;
	packet.y = m_Clients[client_id].y;
	packet.z = m_Clients[client_id].z;

	m_mutexAmmoLock[client_id].lock();
	packet.m_TotalAmmo = m_Clients[client_id].m_TotalAmmo;
	packet.m_CurrentAmmo = m_Clients[client_id].m_CurrentAmmo;
	m_mutexAmmoLock[client_id].unlock();

	SendPacket(client_id, &packet);

	// 나 제외 플레이어에게 입장정보 전송
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (m_Clients[i].m_bUsing && (client_id != i)) {
			//printf("%d에게 %d의 정보를 보낸다\n", i, client_id);
			SendPacket(i, &packet);
		}
	}


	// 다른 클라이언트의 위치 보내주기
	for (int i = 0; i < MAX_PLAYER; ++i) {
		ZeroMemory(&packet, sizeof(packet));
		if (i != client_id) {
			if (m_Clients[i].m_bUsing == true) {
				packet.id = i;
				packet.size = sizeof(SC_PACKET_ENTER_PLAYER);
				packet.type = SC_ENTER_PLAYER;
				m_Clients[i].y = height_map->GetHeight(m_Clients[i].x + DX12_TO_OPGL, m_Clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
				packet.x = m_Clients[i].x;
				packet.y = m_Clients[i].y;
				packet.z = m_Clients[i].z;
				SendPacket(client_id, &packet);
				//printf("%d에게 %d의 정보를 보낸다\n", client_id, i);
			}
		}
	}

}

void ServerFramework::ProcessPacket(int cl_id, char* packet) {
	CS_PACKET_KEYUP* packet_buffer = reinterpret_cast<CS_PACKET_KEYUP*>(packet);
	//printf("[받음-----] %d Pakcet을 %d\n", packet_buffer->type, cl_id);
	switch (packet_buffer->type) {
	case CS_KEY_PRESS_UP:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		m_Clients[cl_id].m_bMoveFoward = true;
		break;
	case CS_KEY_PRESS_DOWN:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		m_Clients[cl_id].m_bMoveBackward = true;
		break;
	case CS_KEY_PRESS_LEFT:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		m_Clients[cl_id].m_bMoveLeft = true;
		break;
	case CS_KEY_PRESS_RIGHT:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		m_Clients[cl_id].m_bMoveRight = true;
		break;
		// 디버그 용임-> 시간 빨리 깎기
	case CS_DEBUG_TIME:
		m_fBoatGenTime += 10;

		break;
	case CS_KEY_PRESS_1:
		m_Clients[cl_id].m_bEquipped = 0;
		break;
	case CS_KEY_PRESS_2:
		m_Clients[cl_id].m_bEquipped = 1;
		break;

	case CS_KEY_PRESS_SHIFT:
		m_Clients[cl_id].m_bRunning = true;
		break;
	case CS_KEY_PRESS_SPACE:
		break;

	case CS_KEY_RELEASE_UP:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		m_Clients[cl_id].m_bMoveFoward = false;
		break;
	case CS_KEY_RELEASE_DOWN:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		m_Clients[cl_id].m_bMoveBackward = false;
		break;
	case CS_KEY_RELEASE_LEFT:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		m_Clients[cl_id].m_bMoveLeft = false;
		break;
	case CS_KEY_RELEASE_RIGHT:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		m_Clients[cl_id].m_bMoveRight = false;
		break;
	case CS_KEY_RELEASE_1:
		break;
	case CS_KEY_RELEASE_2:
		break;
	case CS_KEY_RELEASE_SHIFT:
		m_Clients[cl_id].m_bRunning = false;
		break;
	case CS_KEY_RELEASE_SPACE:
		break;

	case CS_RIGHT_BUTTON_DOWN:
		m_Clients[cl_id].m_bRightClick = true;
		break;
	case CS_RIGHT_BUTTON_UP:
		m_Clients[cl_id].m_bRightClick = false;
		break;
	case CS_RELOAD:
		// AR 장착중인경우
		if (m_Clients[cl_id].m_bEquipped == 0) {
			// 장전하려고 보니 결과가 0 이상
			m_mutexAmmoLock[cl_id].lock();
			if (m_Clients[cl_id].m_TotalAmmo > 0) {
				if (m_Clients[cl_id].m_TotalAmmo - m_Clients[cl_id].m_CurrentAmmo > 0) {
					m_Clients[cl_id].m_TotalAmmo -= (30 - m_Clients[cl_id].m_CurrentAmmo);
					m_Clients[cl_id].m_CurrentAmmo = 30;
					SC_PACKET_AMMO_O packets;
					packets.size = sizeof(SC_PACKET_AMMO_O);
					packets.type = SC_FULLY_AMMO;
					packets.ammo = m_Clients[cl_id].m_CurrentAmmo;
					packets.m_cTotalAmmo = m_Clients[cl_id].m_TotalAmmo;
					SendPacket(cl_id, &packets);
				}
				else if (m_Clients[cl_id].m_TotalAmmo - m_Clients[cl_id].m_CurrentAmmo <= 0) {
					m_Clients[cl_id].m_CurrentAmmo += m_Clients[cl_id].m_TotalAmmo;
					m_Clients[cl_id].m_TotalAmmo = 0;
					SC_PACKET_AMMO_O packets;
					packets.size = sizeof(SC_PACKET_AMMO_O);
					packets.type = SC_FULLY_AMMO;
					packets.ammo = m_Clients[cl_id].m_CurrentAmmo;
					SendPacket(cl_id, &packets);
				}
			}
			m_mutexAmmoLock[cl_id].unlock();
		}
		// SUB 장착중인경우
		else {
			// --------------------------------------------------------------
			// 2018 07 31 : 해야해요! 보조무기
			// --------------------------------------------------------------

			//// 장전하려고 보니 결과가 0 이상
			//if (m_Clients[cl_id].sub_ammo - bullet_counter[cl_id] > 0) {
			//	m_Clients[cl_id].sub_ammo -= bullet_counter[cl_id];
			//	bullet_counter[cl_id] = 0;
			//	printf("장전 완료, 남은 탄창 %d \n", m_Clients[cl_id].sub_ammo);
			//	SC_PACKET_AMMO_O packets;
			//	packets.size = sizeof(SC_PACKET_AMMO_O);
			//	packets.type = SC_FULLY_AMMO;
			//	packets.ammo = bullet_counter[cl_id];
			//	SendPacket(cl_id, &packets);
			//}
			//// 장전하려고 보니 결과가 0 미만
			//else {
			//	bullet_counter[cl_id] -= m_Clients[cl_id].sub_ammo;
			//	m_Clients[cl_id].sub_ammo = 0;
			//	printf("장전 완료, 남은 탄창 %d \n", m_Clients[cl_id].sub_ammo);
			//	SC_PACKET_AMMO_O packets;
			//	packets.size = sizeof(SC_PACKET_AMMO_O);
			//	packets.type = SC_FULLY_AMMO;
			//	packets.ammo = bullet_counter[cl_id];
			//	SendPacket(cl_id, &packets);

			//}

		}
		break;
	case CS_LEFT_BUTTON_DOWN:
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		if (m_Clients[cl_id].m_bEquipped == 0) {
			if (m_Clients[cl_id].m_CurrentAmmo == 0) {
				SC_PACKET_AMMO_O packets;
				packets.size = sizeof(SC_PACKET_AMMO_O);
				packets.type = SC_OUT_OF_AMMO;
				SendPacket(cl_id, &packets);
			}
			else {
				m_Clients[cl_id].m_bLeftClick = true;
				ol_ex[6].evt_type = EVT_BULLET_GENERATE;
				ol_ex[6].shooter_player_id = cl_id;
				//ol_ex[6].elapsed_time = elapsed_time.count();
				PostQueuedCompletionStatus(iocp_handle, 0, 6, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[6]));
			}
		}
		// 보조무기
		else if (m_Clients[cl_id].m_bEquipped == 1) {
			//if (bullet_counter[cl_id] == MAX_AMMO) {
			//	printf("총알 장전 필요\n");
			//	SC_PACKET_AMMO_O packets;
			//	packets.size = sizeof(SC_PACKET_AMMO_O);
			//	packets.type = SC_OUT_OF_AMMO;
			//	SendPacket(cl_id, &packets);
			//}
			//else {
			//	m_Clients[cl_id].m_bLeftClick = true;
			//	ol_ex[6].evt_type = EVT_BULLET_GENERATE;
			//	ol_ex[6].shooter_player_id = cl_id;
			//	//ol_ex[6].elapsed_time = elapsed_time.count();
			//	PostQueuedCompletionStatus(iocp_handle, 0, 6, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[6]));
			//}
		}

		break;
	case CS_LEFT_BUTTON_UP: {
		m_Clients[cl_id].m_bLeftClick = false;
		break;
	}
	case CS_MOUSE_MOVE: {
		m_Clients[cl_id].m_v3LookVec.x = packet_buffer->m_v3LookVec.x;
		m_Clients[cl_id].m_v3LookVec.y = packet_buffer->m_v3LookVec.y;
		m_Clients[cl_id].m_v3LookVec.z = packet_buffer->m_v3LookVec.z;
		SC_PACKET_LOOCVEC packets;
		packets.id = cl_id;
		packets.size = sizeof(SC_PACKET_LOOCVEC);
		packets.type = SC_PLAYER_LOOKVEC;
		packets.m_v3LookVec.x = m_Clients[cl_id].m_v3LookVec.x;
		packets.m_v3LookVec.y = m_Clients[cl_id].m_v3LookVec.y;
		packets.m_v3LookVec.z = m_Clients[cl_id].m_v3LookVec.z;
		// 플레이어가 뒤는 상황

		//if (m_Clients[cl_id].m_bLeftClick) {
		//	packets.player_status = 3;
		//}
		//else if (m_Clients[cl_id].m_bRunning) {
		//	packets.player_status = 2;
		//}
		// 걷지도 뛰지도 않는 상황


		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (m_Clients[i].m_bUsing == true) {
				SendPacket(i, &packets);
			}
		}
		break;
	}
	case CS_PLAYER_READY: {
		int ready_count = 0;
		m_Clients[cl_id].m_bReady = true;
		//if (m_Clients[0].m_bReady && m_Clients[1].m_bReady && m_Clients[2].m_bReady && m_Clients[3].m_bReady) {
		//	GameStart();
		//}
		int iReadyCounter = 0;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (m_Clients[i].m_bUsing && m_Clients[i].m_bReady) {
				iReadyCounter++;
			}
		}
		if (iReadyCounter == MAX_PLAYER) {
			GameStart();
		}
		SC_PACKET_READY packets;
		packets.size = sizeof(SC_PACKET_READY);
		packets.type = SC_PLAYER_READY;
		packets.m_cPlayerNumber = cl_id;

		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (m_Clients[i].m_bUsing)
				SendPacket(i, &packets);
		}

		break;
	}
	case CS_PLAYER_READY_CANCLE: {
		m_Clients[cl_id].m_bReady = false;
		SC_PACKET_READY packets;
		packets.size = sizeof(SC_PACKET_READY);
		packets.type = SC_PLAYER_READY_CANCLE;
		packets.m_cPlayerNumber = cl_id;

		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (m_Clients[i].m_bUsing)
				SendPacket(i, &packets);
		}

		break;
	}
	case CS_TEAM_RED: {
		m_Clients[cl_id].m_Team = e_TeamRed;
		CS_PACKET_TEAM_SELECT packets;
		packets.size = sizeof(CS_PACKET_TEAM_SELECT);
		packets.type = SC_TEAM_RED;
		packets.m_cID = cl_id;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (m_Clients[i].m_bUsing)
				SendPacket(i, &packets);
		}
		break;
	}
	case CS_TEAM_BLUE: {
		m_Clients[cl_id].m_Team = e_TeamBlue;
		CS_PACKET_TEAM_SELECT packets;
		packets.size = sizeof(CS_PACKET_TEAM_SELECT);
		packets.type = SC_TEAM_BLUE;
		packets.m_cID = cl_id;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (m_Clients[i].m_bUsing)
				SendPacket(i, &packets);
		}
		break;
	}
	case CS_MODE_TEAM: {
		SC_PACKET_GAMEMODE packets;
		packets.size = sizeof(SC_PACKET_GAMEMODE);
		packets.type = SC_MODE_TEAM;
		m_bGameMode = false;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (m_Clients[i].m_bUsing) {
				SendPacket(i, &packets);
			}
		}
		break;
	}
	case CS_MODE_MELEE: {
		SC_PACKET_GAMEMODE packets;
		packets.size = sizeof(SC_PACKET_GAMEMODE);
		packets.type = SC_MODE_MELEE;
		m_bGameMode = true;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (m_Clients[i].m_bUsing) {
				SendPacket(i, &packets);
			}
		}
		break;
	}
	case CS_ENTER_LOBBY: {
		SC_PACKET_ENTER_LOBBY packets;
		packets.size = sizeof(SC_PACKET_ENTER_LOBBY);
		packets.type = SC_ENTER_LOBBY;
		SendPacket(cl_id, &packets);
		break;
	}
	case CS_RESTART_GAME: {
		for (int i = 0; i < MAX_PLAYER; ++i) {
			m_Clients[i].x = 0.f;
			m_Clients[i].z = 0.f;
			m_Clients[i].y = height_map->GetHeight(m_Clients[i].x + DX12_TO_OPGL, m_Clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
			m_Clients[i].m_fhp = 100.f;
			m_Clients[i].m_fStamina = 100.f;
			m_Clients[i].m_CurrentAmmo = 30;
			m_Clients[i].m_TotalAmmo = 90;
			m_Clients[i].m_bReady = false;
			m_Clients[i].m_Team = e_NoTeam;
			m_bWhoDie[i] = false;
			m_bDieSender[i] = false;
			for (int j = 0; j < 4; ++j) {
				m_Clients[i].m_bPlayerBoatParts[j] = false;
			}
		}
		m_bIsBoatGen = false;
		m_bGameStart = false;
		m_bIsAmmoGen = false;
		m_fBoatGenTime = 0.f;

		break;
	}
	case CS_ASSENBLE_PARTS: {
		//printf("조립 가즈아\n");
		// 게임 Status Update
		// 이걸 F키 눌렀을때 해야함 특정지역에서 
		if (m_bGameMode == false) {
			int iTeamRed = 0;
			int iTeamBlue = 0;
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (m_Clients[i].m_Team == e_TeamRed) {
					for (int j = 0; j < 4; ++j) {
						if (m_Clients[i].m_bPlayerBoatParts[j]) {
							iTeamRed++;
						}
					}
				}
				else if (m_Clients[i].m_Team == e_TeamBlue) {
					for (int j = 0; j < 4; ++j) {
						if (m_Clients[i].m_bPlayerBoatParts[j]) {
							iTeamBlue++;
						}
					}
				}
			}
			// 보트 부품 다 모은경우
			// Red 승리
			if (iTeamRed == 4) {
				//printf("레드 승리 \n");
				//m_bGameStart = false;
				SC_PACKET_RESULT packets;
				packets.size = sizeof(SC_PACKET_RESULT);
				packets.type = SC_RESULT;
				packets.m_cVictoryTeam = 5;
				for (int k = 0; k < MAX_PLAYER; ++k) {
					SendPacket(k, &packets);
				}

			}
			// Blue 승리
			else if (iTeamBlue == 4) {
				//printf("블루 승리 \n");
				//m_bGameStart = false;
				SC_PACKET_RESULT packets;
				packets.size = sizeof(SC_PACKET_RESULT);
				packets.type = SC_RESULT;
				packets.m_cVictoryTeam = 6;
				for (int k = 0; k < MAX_PLAYER; ++k) {
					SendPacket(k, &packets);
				}

			}
		}
		else {
			int iPlayerCounter[MAX_PLAYER] = { 0 };

			for (int i = 0; i < MAX_PLAYER; ++i) {
				for (int j = 0; j < 4; ++j) {
					if (m_Clients[i].m_bPlayerBoatParts[j]) {
						iPlayerCounter[i]++;
					}
				}

				if (iPlayerCounter[i] == 4) {
					//printf("%d 플레이어 승리\n", i + 1);
					//m_bGameStart = false;
					SC_PACKET_RESULT packets;
					packets.size = sizeof(SC_PACKET_RESULT);
					packets.type = SC_RESULT;
					packets.m_cVictoryTeam = i + 1;
					for (int k = 0; k < MAX_PLAYER; ++k) {
						SendPacket(k, &packets);
					}

				}
			}


		}
		break;
	}
	}

}

void ServerFramework::GameStart() {
	printf("게임 시작\n");
	SC_PACKET_START packets;
	packets.size = sizeof(SC_PACKET_START);
	packets.type = SC_GAME_START;
	for (int i = 0; i < MAX_PLAYER; ++i) {
		packets.m_bPlayerTeam[i] = m_Clients[i].m_Team;
	}
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (m_Clients[i].m_bUsing)
			SendPacket(i, &packets);
	}

	m_Clients[0].x = -195.f;
	m_Clients[0].z = -120.f;
	m_Clients[0].y = height_map->GetHeight(m_Clients[0].x + DX12_TO_OPGL, m_Clients[0].z + DX12_TO_OPGL) + PLAYER_HEIGHT;

	m_Clients[1].x = -130.f;
	m_Clients[1].z = 138.f;
	m_Clients[1].y = height_map->GetHeight(m_Clients[1].x + DX12_TO_OPGL, m_Clients[1].z + DX12_TO_OPGL) + PLAYER_HEIGHT;

	m_Clients[2].x = 128.f;
	m_Clients[2].z = 128.f;
	m_Clients[2].y = height_map->GetHeight(m_Clients[2].x + DX12_TO_OPGL, m_Clients[2].z + DX12_TO_OPGL) + PLAYER_HEIGHT;

	m_Clients[3].x = 117.f;
	m_Clients[3].z = -136.f;
	m_Clients[3].y = height_map->GetHeight(m_Clients[3].x + DX12_TO_OPGL, m_Clients[3].z + DX12_TO_OPGL) + PLAYER_HEIGHT;

	for (int i = 0; i < MAX_PLAYER; ++i) {
		m_Clients[i].m_fStamina = 100.f;
		ol_ex[i].evt_type = EVT_PLAYER_POS_SEND;
		PostQueuedCompletionStatus(iocp_handle, 0, i, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[i]));
	}
	for (int i = 0; i < MAX_PLAYER; ++i) {
		for (int j = 1; j <= MAX_AMMO; ++j) {
			bullets[i][j].m_bUsing = false;
		}
	}

	m_bIsBoatGen = true;
	m_bGameStart = true;
	m_bIsAmmoGen = true;
	m_fBoatGenTime = 0.f;
}

void ServerFramework::WorkerThread() {
	unsigned long data_size = 0;
	unsigned long long client_id = 0;

	WSAOVERLAPPED* overlapped;

	while (true) {
		bool retval = GetQueuedCompletionStatus(iocp_handle, &data_size,
			&client_id, &overlapped, INFINITE);
		if (retval == FALSE) {
			printf("[WorkerThread::GQCS] 에러 ClientID : %d\n", client_id);
			if (data_size == 0) {
				DisconnectPlayer(client_id);
				continue;
			}
		}
		OverlappedExtensionSet* overlapped_buffer = reinterpret_cast<OverlappedExtensionSet*>(overlapped);
		if (overlapped_buffer->evt_type == EVT_RECV_PACKET) {
			int recved_size = data_size;
			char* ptr = overlapped_buffer->io_buffer;
			while (recved_size > 0) {
				if (m_Clients[client_id].m_bPacketSize == 0) {
					m_Clients[client_id].m_bPacketSize = ptr[0];
				}
				int remain = m_Clients[client_id].m_bPacketSize - m_Clients[client_id].m_bPrevPacketSize;
				if (remain <= recved_size) {
					memcpy(m_Clients[client_id].m_arrPrevPacket + m_Clients[client_id].m_bPrevPacketSize,
						ptr,
						remain);
					ProcessPacket(static_cast<int>(client_id), m_Clients[client_id].m_arrPrevPacket);
					recved_size -= remain;
					ptr += remain;
					m_Clients[client_id].m_bPacketSize = 0;
					m_Clients[client_id].m_bPrevPacketSize = 0;
				}
				else {
					memcpy(m_Clients[client_id].m_arrPrevPacket + m_Clients[client_id].m_bPrevPacketSize,
						ptr,
						recved_size);
					recved_size -= recved_size;
					ptr += recved_size;
				}
			}

			unsigned long rflag = 0;
			ZeroMemory(&overlapped_buffer->wsa_over, sizeof(WSAOVERLAPPED));
			int retval = WSARecv(m_Clients[client_id].s, &overlapped_buffer->wsabuf, 1, NULL, &rflag, &overlapped_buffer->wsa_over, NULL);
			if (retval != 0) {
				int err_no = WSAGetLastError();
				if (err_no != WSA_IO_PENDING) {
					ErrorDisplay("Error in WorkerThread(Recv) : %d \n", err_no);
				}
			}

		}
		else if (overlapped_buffer->evt_type == EVT_PLAYER_HP_UPDATE) {
			// Melee Mode
			int iAliveCounter = MAX_PLAYER;
			if (m_bGameMode) {
				for (int i = 0; i < MAX_PLAYER; ++i) {
					if (m_Clients[i].m_fhp <= 0.f) {
						iAliveCounter--;
					}
				}
				
				if (iAliveCounter == 1) {
					//m_bGameStart = false;
					for (int j = 0; j < MAX_PLAYER; ++j) {
						// 체력이 양수인 플레이어가 최종 생존자라는 뜻
						if (m_Clients[j].m_fhp > 0.f) {
							printf("%d Player Win\n", j + 1);
							SC_PACKET_RESULT packets;
							packets.size = sizeof(SC_PACKET_RESULT);
							packets.type = SC_RESULT;
							packets.m_cVictoryTeam = j + 1;
							for (int k = 0; k < MAX_PLAYER; ++k) {
								SendPacket(k, &packets);
							}
						}
					}
				}
			}
			// Team Mode
			else {
				int iDeathCountRed = 0;
				int iDeathCountBlue = 0;
				for (int j = 0; j < MAX_PLAYER; ++j) {
					if (m_Clients[j].m_fhp <= 0.f && m_Clients[j].m_Team == e_TeamRed) {
						iDeathCountRed++;
					}
					if (m_Clients[j].m_fhp <= 0.f && m_Clients[j].m_Team == e_TeamBlue) {
						iDeathCountBlue++;
					}
				}
				//iAliveCounterRed--;
				if (iDeathCountRed == MAX_PLAYER / 2) {
					//m_bGameStart = false;
					//왜?
					printf("Blue Team Win\n");
					SC_PACKET_RESULT packets;
					packets.size = sizeof(SC_PACKET_RESULT);
					packets.type = SC_RESULT;
					packets.m_cVictoryTeam = 6;
					for (int k = 0; k < MAX_PLAYER; ++k) {
						SendPacket(k, &packets);
					}
				}
				if (iDeathCountBlue == MAX_PLAYER / 2) {
					//m_bGameStart = false;
					//왜?
					printf("Red Team Win\n");
					SC_PACKET_RESULT packets;
					packets.size = sizeof(SC_PACKET_RESULT);
					packets.type = SC_RESULT;
					packets.m_cVictoryTeam = 5;
					for (int k = 0; k < MAX_PLAYER; ++k) {
						SendPacket(k, &packets);
					}
				}
			}

			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (m_Clients[i].m_fhp <= 0.f && m_bDieSender[i] == false) {
					//printf("%d 플레이어 Die\n",i);
					SC_PACKET_DIE packets;
					packets.size = sizeof(SC_PACKET_DIE);
					packets.type = SC_PLAYER_DIE;
					packets.m_cDiePlayer = i;

					// 죽은 플레이어가 아이템을 가지고 있으면 해당위치에 Drop
					for (int j = 0; j < 4; ++j) {
						// 만약 가지고 있다면 
						if (m_Clients[i].m_bPlayerBoatParts[j] == true) {
							m_itemBoat[j].m_bUse = true;
							m_itemBoat[j].x = m_Clients[i].x;
							m_itemBoat[j].y = m_Clients[i].y;
							m_itemBoat[j].z = m_Clients[i].z;
							m_Clients[i].m_bPlayerBoatParts[j] = false;
							packets.m_bBoatItem[j] = true;
							printf("죽고 %d 아이템 드랍\n", j);
						}
						//
					}

					packets.m_fDiePosX = m_Clients[i].x;
					packets.m_fDiePosY = m_Clients[i].y;
					packets.m_fDiePosZ = m_Clients[i].z;
					for (int j = 0; j < MAX_PLAYER; ++j) {
						SendPacket(j, &packets);
					}
					m_bDieSender[i] = true;
				}
				else {
					// 플레이어가 살아있으면 Stamina 회복해줌.
					if (m_Clients[i].m_bMoveBackward == false &&
						m_Clients[i].m_bMoveFoward == false &&
						m_Clients[i].m_bMoveLeft == false &&
						m_Clients[i].m_bMoveRight == false &&
						m_Clients[i].m_fStamina < 95.f) {
						m_Clients[i].m_fStamina += 5.f;
						SC_PACKET_STAMINA packets;
						packets.size = sizeof(SC_PACKET_STAMINA);
						packets.type = SC_STAMINA;
						packets.m_fStamina = m_Clients[i].m_fStamina;
						packets.m_cID = i;
						SendPacket(i, &packets);
					}

				}
				if (m_Clients[i].m_bUsing && m_BoatGenedMap[0]) {
					if (-256.f < m_Clients[i].x&&m_Clients[i].x <= 0.f) {
						if (-256.f <= m_Clients[i].z && m_Clients[i].z <= 0.f) {
							m_Clients[i].m_fhp -= 1.f;
							SC_PACKET_PLAYER_HP_UPDATE packets;
							packets.size = sizeof(SC_PACKET_PLAYER_HP_UPDATE);
							packets.type = SC_PLAYER_HP_UPDATE;
							packets.m_fHp = m_Clients[i].m_fhp;
							packets.m_cPlayerID = i;
							printf("%d Player HP : %f \n", packets.m_cPlayerID, packets.m_fHp);
							SendPacket(i, &packets);
						}
					}
				}
				if (m_Clients[i].m_bUsing && m_BoatGenedMap[1]) {
					if (-256.f < m_Clients[i].x&&m_Clients[i].x <= 0) {
						if (0 <= m_Clients[i].z && m_Clients[i].z <= 256.f) {
							m_Clients[i].m_fhp -= 1.f;
							SC_PACKET_PLAYER_HP_UPDATE packets;
							packets.size = sizeof(SC_PACKET_PLAYER_HP_UPDATE);
							packets.type = SC_PLAYER_HP_UPDATE;
							packets.m_fHp = m_Clients[i].m_fhp;
							packets.m_cPlayerID = i;
							printf("%d Player HP : %f \n", packets.m_cPlayerID, packets.m_fHp);
							SendPacket(i, &packets);
						}
					}
				}
				if (m_Clients[i].m_bUsing && m_BoatGenedMap[2]) {
					if (0.f < m_Clients[i].x&&m_Clients[i].x <= 256.f) {
						if (0 <= m_Clients[i].z && m_Clients[i].z <= 256.f) {
							m_Clients[i].m_fhp -= 1.f;
							SC_PACKET_PLAYER_HP_UPDATE packets;
							packets.size = sizeof(SC_PACKET_PLAYER_HP_UPDATE);
							packets.type = SC_PLAYER_HP_UPDATE;
							packets.m_fHp = m_Clients[i].m_fhp;
							packets.m_cPlayerID = i;
							printf("%d Player HP : %f \n", packets.m_cPlayerID, packets.m_fHp);
							SendPacket(i, &packets);
						}
					}
				}
				if (m_Clients[i].m_bUsing && m_BoatGenedMap[3]) {
					if (0.f < m_Clients[i].x&&m_Clients[i].x <= 256.f) {
						if (-256.f <= m_Clients[i].z && m_Clients[i].z <= 0.f) {
							m_Clients[i].m_fhp -= 1.f;
							SC_PACKET_PLAYER_HP_UPDATE packets;
							packets.size = sizeof(SC_PACKET_PLAYER_HP_UPDATE);
							packets.type = SC_PLAYER_HP_UPDATE;
							packets.m_fHp = m_Clients[i].m_fhp;
							packets.m_cPlayerID = i;
							printf("%d Player HP : %f \n", packets.m_cPlayerID, packets.m_fHp);
							SendPacket(i, &packets);
						}
					}
				}
			}
		}
		else if (overlapped_buffer->evt_type == EVT_AMMO_ITEM_GEN) {
			int iDice = 0;
			iDice = rand() % 8;
			switch (iDice) {
			case 0:
				m_itemAmmo[0].m_bUse = true;
				m_itemAmmo[0].m_ItemType = 4;
				m_itemAmmo[0].x = -204.f;
				m_itemAmmo[0].y = 79.f;
				m_itemAmmo[0].z = -186.f;
				break;
			case 1:
				m_itemAmmo[1].m_bUse = true;
				m_itemAmmo[1].m_ItemType = 4;
				m_itemAmmo[1].x = -195.f;
				m_itemAmmo[1].y = 72.f;
				m_itemAmmo[1].z = -73.f;
				break;
			case 2:
				m_itemAmmo[2].m_bUse = true;
				m_itemAmmo[2].m_ItemType = 4;
				m_itemAmmo[2].x = -160.f;
				m_itemAmmo[2].y = 81.f;
				m_itemAmmo[2].z = 71.f;
				break;
			case 3:
				m_itemAmmo[3].m_bUse = true;
				m_itemAmmo[3].m_ItemType = 4;
				m_itemAmmo[3].x = -167.f;
				m_itemAmmo[3].y = 45.f;
				m_itemAmmo[3].z = 201.f;
				break;
			case 4:
				m_itemAmmo[4].m_bUse = true;
				m_itemAmmo[4].m_ItemType = 4;
				m_itemAmmo[4].x = 79.f;
				m_itemAmmo[4].y = 43.f;
				m_itemAmmo[4].z = 191.f;
				break;
			case 5:
				m_itemAmmo[5].m_bUse = true;
				m_itemAmmo[5].m_ItemType = 4;
				m_itemAmmo[5].x = 162.f;
				m_itemAmmo[5].y = 61.f;
				m_itemAmmo[5].z = 95.f;
				break;
			case 6:
				m_itemAmmo[6].m_bUse = true;
				m_itemAmmo[6].m_ItemType = 4;
				m_itemAmmo[6].x = 170.f;
				m_itemAmmo[6].y = 57.f;
				m_itemAmmo[6].z = -58.f;
				break;
			case 7:
				m_itemAmmo[7].m_bUse = true;
				m_itemAmmo[7].m_ItemType = 4;
				m_itemAmmo[7].x = 158.f;
				m_itemAmmo[7].y = 45.f;
				m_itemAmmo[7].z = -183.f;
				break;
			}
			printf("Ammo : No.%d 생성 [%f, %f, %f]\n", iDice,
				m_itemAmmo[iDice].x,
				m_itemAmmo[iDice].y,
				m_itemAmmo[iDice].z);

			SC_PACKET_ITEM_GEN packets;
			packets.size = sizeof(SC_PACKET_ITEM_GEN);
			packets.type = SC_AMMO_ITEM_GEN;
			packets.item_type = 4;
			packets.m_cItemID = iDice;
			packets.x = m_itemAmmo[iDice].x;
			packets.y = m_itemAmmo[iDice].y;
			packets.z = m_itemAmmo[iDice].z;
			for (int i = 0; i < MAX_PLAYER; ++i) {
				SendPacket(i, &packets);
			}

		}
		else if (overlapped_buffer->evt_type == EVT_BOAT_ITEM_GEN) {

			if (m_iDiceCounter < 4) {
				m_mutexBoatItem.lock();
				int iDice = 0;
				while (true) {
					iDice = rand() % 4;
					// 랜덤하게 접근한 번호가 사용중이면  닫시 랜덤 
					if (m_BoatGenedMap[iDice] == true) {
						continue;
					}
					else {
						m_iDiceCounter++;
						m_itemBoat[iDice].m_bUse = true;
						m_itemBoat[iDice].m_ItemType = iDice;
						m_BoatGenedMap[iDice] = true;

						break;
					}
				}
				switch (iDice) {
				case 0:
					m_itemBoat[iDice].x = -185.f;
					m_itemBoat[iDice].y = 93.f;
					m_itemBoat[iDice].z = -208.f;
					break;
				case 1:
					m_itemBoat[iDice].x = -110.f;
					m_itemBoat[iDice].y = 71.f;
					m_itemBoat[iDice].z = 138.f;
					break;
				case 2:
					m_itemBoat[iDice].x = 113.f;
					m_itemBoat[iDice].y = 89.f;
					m_itemBoat[iDice].z = 160.f;
					break;
				case 3:
					m_itemBoat[iDice].x = 81;
					m_itemBoat[iDice].y = 79.f;
					m_itemBoat[iDice].z = -129.f;
					break;
				}

				SC_PACKET_ITEM_GEN packets;
				packets.size = sizeof(SC_PACKET_ITEM_GEN);
				packets.type = SC_BOAT_ITEM_GEN;
				packets.item_type = m_itemBoat[iDice].m_ItemType;
				packets.x = m_itemBoat[iDice].x;
				packets.y = m_itemBoat[iDice].y;
				packets.z = m_itemBoat[iDice].z;
				printf("%d Type 보냈다\n", packets.item_type);
				for (int i = 0; i < MAX_PLAYER; ++i) {
					SendPacket(i, &packets);
				}
				m_mutexBoatItem.unlock();
			}
			if (m_iDiceCounter == 2) {
				SC_PACKET_WEATHER packets;
				packets.size = sizeof(SC_PACKET_WEATHER);
				packets.type = SC_WEATHER_CHANGE;
				for (int i = 0; i < MAX_PLAYER; ++i) {
					SendPacket(i, &packets);
				}
			}
		}
		// TimerThread에서 호출
		// 1/20 마다 모든 플레이어에게 정보 전송
		else if (overlapped_buffer->evt_type == EVT_PLAYER_POS_SEND) {
			for (int i = 0; i < MAX_PLAYER; ++i) {
				m_Clients[i].m_mutexServerLock.lock();
				if (m_Clients[i].m_bMoveFoward) {
					if (m_Clients[i].m_bRunning && m_Clients[i].m_fStamina > 0.f) {
						m_Clients[i].m_fStamina -= 0.1f;
						if (31.f <= m_Clients[i].x &&m_Clients[i].x < 34.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && 26.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -153.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -180.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -154.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (m_Clients[i].x > 255.f) {
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							m_Clients[i].z += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
					}
					// 걷기 
					else {
						if (31.f <= m_Clients[i].x &&m_Clients[i].x < 34.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && 26.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -153.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -180.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -154.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (m_Clients[i].x > 255.f) {
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							m_Clients[i].z += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}

					}
				}
				if (m_Clients[i].m_bMoveBackward) {

					if (m_Clients[i].m_bRunning && m_Clients[i].m_fStamina > 0.f) {
						m_Clients[i].z += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						m_Clients[i].x += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						m_Clients[i].m_fStamina -= 0.1f;

						//if (31.f <= m_Clients[i].x &&m_Clients[i].x < 34.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28) {
						//	//printf("1번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && 26.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
						//	//printf("2번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f,0.f,1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -153.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
						//	//printf("3번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { -1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= -89.f) {
						//	//printf("4번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f,0.f, -1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (30.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
						//	//printf("5번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -180.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
						//	//printf("6번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -154.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
						//	//printf("7번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (m_Clients[i].x > 255.f) {
						//	printf("빠꾸치다가 넘었다\n");
						//	glm::vec3 v3Normal = { -1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-255.f > m_Clients[i].x) {
						//	glm::vec3 v3Normal = { 1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-255.f > m_Clients[i].z) {
						//	glm::vec3 v3Normal = { 0.f,0.f,1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (255.f < m_Clients[i].z) {
						//	glm::vec3 v3Normal = { 0.f,0.f,-1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else {
						//	m_Clients[i].z -= (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}

					}
					// 걷기
					else {
						m_Clients[i].z += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						m_Clients[i].x += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//if (31.f <= m_Clients[i].x &&m_Clients[i].x < 34.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28) {
						//	//printf("1번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && 26.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
						//	//printf("2번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f,0.f,1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -153.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
						//	//printf("3번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { -1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= -89.f) {
						//	//printf("4번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f,0.f, -1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (30.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
						//	//printf("5번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -180.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
						//	//printf("6번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -154.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
						//	//printf("7번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (m_Clients[i].x > 255.f) {
						//	glm::vec3 v3Normal = { -1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-255.f > m_Clients[i].x) {
						//	glm::vec3 v3Normal = { 1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-255.f > m_Clients[i].z) {
						//	glm::vec3 v3Normal = { 0.f,0.f,1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (255.f < m_Clients[i].z) {
						//	glm::vec3 v3Normal = { 0.f,0.f,-1.f };
						//	glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
						//	m_Clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else {
						//	m_Clients[i].z -= (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	m_Clients[i].x -= (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}

					}

				}
				if (m_Clients[i].m_bMoveLeft) {
					if (m_Clients[i].m_bRunning && m_Clients[i].m_fStamina > 0.f) {
						//m_Clients[i].z += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//m_Clients[i].x += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						m_Clients[i].m_fStamina -= 0.1f;

						if (31.f <= m_Clients[i].x &&m_Clients[i].x < 34.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && 26.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -153.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -180.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -154.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (m_Clients[i].x > 255.f) {
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							m_Clients[i].z += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}

					}
					else {
						//m_Clients[i].z += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//m_Clients[i].x += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);

						if (31.f <= m_Clients[i].x &&m_Clients[i].x < 34.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && 26.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -153.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -180.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -154.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (m_Clients[i].x > 255.f) {
							printf("빠꾸치다가 넘었다\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							m_Clients[i].z += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}

					}
				}
				if (m_Clients[i].m_bMoveRight) {
					if (m_Clients[i].m_bRunning && m_Clients[i].m_fStamina > 0.f) {
						//m_Clients[i].z += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//m_Clients[i].x += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						m_Clients[i].m_fStamina -= 0.1f;

						if (31.f <= m_Clients[i].x &&m_Clients[i].x < 34.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && 26.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -153.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -180.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -154.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (m_Clients[i].x > 255.f) {
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							//m_Clients[i].z += -METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							//m_Clients[i].x += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);

							m_Clients[i].z += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);

						}

					}
					else {
						//m_Clients[i].z += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//m_Clients[i].x += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);

						if (31.f <= m_Clients[i].x &&m_Clients[i].x < 34.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && 26.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -153.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -92.f <= m_Clients[i].z && m_Clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= 33.f && -180.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= m_Clients[i].x && m_Clients[i].x <= -154.f && -256.f <= m_Clients[i].z && m_Clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (m_Clients[i].x > 255.f) {
							printf("빠꾸치다가 넘었다\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < m_Clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = m_Clients[i].m_v3LookVec - v3Normal * (glm::dot(m_Clients[i].m_v3LookVec, v3Normal));
							m_Clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							//m_Clients[i].z += -METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							//m_Clients[i].x += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].z += (-1) * METER_PER_PIXEL * m_Clients[i].m_v3LookVec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
							m_Clients[i].x += METER_PER_PIXEL * m_Clients[i].m_v3LookVec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);

						}

					}
				}

				m_Clients[i].m_mutexServerLock.unlock();
			}

			if (m_Clients[client_id].m_bUsing) {
				SC_PACKET_POS packets;
				packets.id = client_id;
				packets.size = sizeof(SC_PACKET_POS);
				packets.type = SC_POS;
				m_Clients[client_id].y = height_map->GetHeight(m_Clients[client_id].x + DX12_TO_OPGL, m_Clients[client_id].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
				packets.x = m_Clients[client_id].x;
				packets.y = m_Clients[client_id].y;
				packets.z = m_Clients[client_id].z;
				packets.m_fStamina = m_Clients[client_id].m_fStamina;
				for (int i = 0; i < MAX_PLAYER; ++i) {
					if (m_Clients[i].m_bUsing == true) {
						SendPacket(i, &packets);
					}
				}
				ZeroMemory(overlapped_buffer, sizeof(OverlappedExtensionSet));
			}
		}
		else if (overlapped_buffer->evt_type == EVT_COLLISION) {
			// 보트아이템과 플레이어
			//m_mutexBoatItem.lock();
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (m_Clients[i].m_bUsing) {
					for (int j = 0; j < 4; ++j) {
						if (m_itemBoat[j].m_bUse && m_Clients[i].m_fhp > 0.f) {
							float fDist =
								(m_itemBoat[j].x - m_Clients[i].x)*(m_itemBoat[j].x - m_Clients[i].x) +
								(m_itemBoat[j].y - m_Clients[i].y)*(m_itemBoat[j].y - m_Clients[i].y) +
								(m_itemBoat[j].z - m_Clients[i].z)*(m_itemBoat[j].z - m_Clients[i].z);
							float fDistOrigin = (RAD_PLAYER + RAD_ITEM) * (RAD_PLAYER + RAD_ITEM);
							// 충돌
							if (fDist < fDistOrigin) {
								SC_PACKET_GET_ITEM packets;
								packets.size = sizeof(SC_PACKET_GET_ITEM);
								packets.type = SC_PLAYER_GET_ITEM;
								packets.m_cItemType = j;
								packets.m_cGetterID = i;
								m_Clients[i].m_bPlayerBoatParts[j] = true;
								m_itemBoat[j].m_bUse = false;
								//printf("%d플레이어와 %d아이템 충돌\n", i, j);
								for (int k = 0; k < MAX_PLAYER; ++k) {
									SendPacket(k, &packets);
								}
							}
						}
					}
				}
			}
			//m_mutexBoatItem.unlock();

			// Ammo 아이템과 플레이어
			for (int i = 0; i < MAX_PLAYER; ++i) {
				for (int j = 0; j < 8; ++j) {
					if (m_Clients[i].m_bUsing && m_itemAmmo[j].m_bUse) {
						float fDist =
							(m_itemAmmo[j].x - m_Clients[i].x)*(m_itemAmmo[j].x - m_Clients[i].x) +
							(m_itemAmmo[j].y - m_Clients[i].y)*(m_itemAmmo[j].y - m_Clients[i].y) +
							(m_itemAmmo[j].z - m_Clients[i].z)*(m_itemAmmo[j].z - m_Clients[i].z);
						float fDistOrigin = (RAD_PLAYER + RAD_AMMO_ITEM) * (RAD_PLAYER + RAD_AMMO_ITEM);
						// 충돌
						if (fDist < fDistOrigin) {
							SC_PACKET_GET_ITEM packets;
							packets.size = sizeof(SC_PACKET_GET_ITEM);
							packets.type = SC_PLAYER_GET_ITEM;
							// 4 는 무조건 Ammo 아이템이다. 
							packets.m_cItemType = 4;
							packets.m_cAmmoItemID = j;
							packets.m_cGetterID = i;
							//m_Clients[i].m_bPlayerBoatParts[j] = true;
							m_Clients[i].m_TotalAmmo = 90;
							m_itemAmmo[j].m_bUse = false;
							//printf("%d플레이어와 %d아이템 충돌\n", i, j);
							for (int k = 0; k < MAX_PLAYER; ++k) {
								SendPacket(k, &packets);
							}

						}

					}
				}
			}

			// 총알과 플레이어
			for (int i = 0; i < MAX_PLAYER; ++i) {
				m_mutexBulletLock[i].lock();
				for (int j = 1; j <= MAX_AMMO; ++j) {
					for (int k = 0; k < MAX_PLAYER; ++k) {
						// i 플레이어의 총알과 다른 플레이어들을 서로 충돌체크 
						//
						//	i							k
						// 총알 쏘는 플레이어와 총알 맞는 플레이어 모두 존재하고 
						// 플레이어가 발사한 총알까지 존재 해야 이 함수 실행
						if (i == k)continue;
						if (m_bGameMode == 0) {
							if (m_Clients[i].m_bUsing && m_Clients[k].m_bUsing && bullets[i][j].m_bUsing &&
								m_Clients[i].m_Team != m_Clients[k].m_Team) {
								float fDist =
									(bullets[i][j].x - m_Clients[k].x)*(bullets[i][j].x - m_Clients[k].x) +
									(bullets[i][j].y - m_Clients[k].y)*(bullets[i][j].y - m_Clients[k].y) +
									(bullets[i][j].z - m_Clients[k].z)*(bullets[i][j].z - m_Clients[k].z);
								float fDistOrigin = (RAD_PLAYER + RAD_BULLET) * (RAD_PLAYER + RAD_BULLET);
								// 충돌
								if (fDist < fDistOrigin) {
									SC_PACKET_HIT packets;
									packets.size = sizeof(SC_PACKET_HIT);
									packets.type = SC_HIT;
									m_Clients[k].m_fhp -= 10.f;
									packets.m_fHp = m_Clients[k].m_fhp;
									packets.m_cBulletNumber = j;
									packets.m_cShooterID = i;
									packets.m_cHitID = k;
									bullets[i][j].m_bUsing = false;
									//printf("%d플레이어와 %d플레이어의 총알 충돌\n", k, i);
									//printf("후 HP : %f\n", packets.m_fHp);
									for (int l = 0; l < MAX_PLAYER; ++l) {
										SendPacket(l, &packets);
									}
								}
							}
						}
						else {
							if (m_Clients[i].m_bUsing && m_Clients[k].m_bUsing && bullets[i][j].m_bUsing) {
								float fDist =
									(bullets[i][j].x - m_Clients[k].x)*(bullets[i][j].x - m_Clients[k].x) +
									(bullets[i][j].y - m_Clients[k].y)*(bullets[i][j].y - m_Clients[k].y) +
									(bullets[i][j].z - m_Clients[k].z)*(bullets[i][j].z - m_Clients[k].z);
								float fDistOrigin = (RAD_PLAYER + RAD_BULLET) * (RAD_PLAYER + RAD_BULLET);
								// 충돌
								if (fDist < fDistOrigin) {
									SC_PACKET_HIT packets;
									packets.size = sizeof(SC_PACKET_HIT);
									packets.type = SC_HIT;
									m_Clients[k].m_fhp -= 10.f;
									packets.m_fHp = m_Clients[k].m_fhp;
									packets.m_cBulletNumber = j;
									packets.m_cShooterID = i;
									packets.m_cHitID = k;
									bullets[i][j].m_bUsing = false;
									//printf("%d플레이어와 %d플레이어의 총알 충돌\n", k, i);
									//printf("후 HP : %f\n", packets.m_fHp);
									for (int l = 0; l < MAX_PLAYER; ++l) {
										SendPacket(l, &packets);
									}
								}
							}
						}
					}
				}
				m_mutexBulletLock[i].unlock();
			}



			// 지면과 충돌
			for (int i = 0; i < MAX_PLAYER; ++i) {
				m_mutexBulletLock[i].lock();
				for (int j = 1; j <= MAX_AMMO; ++j) {
					if (bullets[i][j].m_bUsing) {
						if (bullets[i][j].y < height_map->GetHeight(bullets[i][j].x + DX12_TO_OPGL, bullets[i][j].z + DX12_TO_OPGL)) {
							if (m_Clients[i].m_bUsing) {
								SC_PACKET_COLLISION_TB packets;
								packets.size = sizeof(SC_PACKET_COLLISION_TB);
								packets.type = SC_COLLISION_TB;
								packets.m_cBulletID = j;
								packets.m_cPlayerID = i;
								bullets[i][j].m_bUsing = false;
								SendPacket(i, &packets);
								continue;
							}
						}
					}
				}
				m_mutexBulletLock[i].unlock();
			}
		}
		else if (overlapped_buffer->evt_type == EVT_BULLET_GENERATE) {
			int shooter_id = overlapped_buffer->shooter_player_id;
			if (m_Clients[shooter_id].m_bEquipped == 0 && m_Clients[shooter_id].m_bUsing && m_Clients[shooter_id].m_fhp > 0.f) {
				m_mutexBulletLock[shooter_id].lock();
				if (m_Clients[shooter_id].m_CurrentAmmo == 1) {
					for (int d = 1; d <= MAX_AMMO; ++d) {
						bullets[shooter_id][d].m_bUsing = false;
					}
				}
				bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].x = m_Clients[shooter_id].x;
				bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].y = m_Clients[shooter_id].y - 5.f;
				bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].z = m_Clients[shooter_id].z;
				bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].m_v3LookVec.x = m_Clients[shooter_id].m_v3LookVec.x;
				bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].m_v3LookVec.y = m_Clients[shooter_id].m_v3LookVec.y;
				bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].m_v3LookVec.z = m_Clients[shooter_id].m_v3LookVec.z;
				bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].m_fBallisticsTime = 0.f;
				bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].m_bUsing = true;
				m_mutexBulletLock[shooter_id].unlock();
				//printf("%d가 발사한 총알 : %d [X : %f, Y : %f, Z : %f, InUse : %d ]\n", shooter_id, m_Clients[shooter_id].m_CurrentAmmo,
				//	bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].x,
				//	bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].y,
				//	bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].z,
				//	bullets[shooter_id][m_Clients[shooter_id].m_CurrentAmmo].m_bUsing);
				//printf("실제 클라이언트 위치 [%f, %f, %f] \n", m_Clients[shooter_id].x, m_Clients[shooter_id].y, m_Clients[shooter_id].z);
				m_Clients[shooter_id].m_CurrentAmmo--;
				// 남은 탄창 보내주기 
				SC_PACKET_AMMO packets;
				packets.size = sizeof(SC_PACKET_AMMO);
				packets.type = SC_AMMO;
				m_mutexAmmoLock[shooter_id].lock();
				packets.m_CurrentAmmo = m_Clients[shooter_id].m_CurrentAmmo;
				packets.m_TotalAmmo = m_Clients[shooter_id].m_TotalAmmo;
				m_mutexAmmoLock[shooter_id].unlock();
				SendPacket(shooter_id, &packets);
			}
		}
		else if (overlapped_buffer->evt_type == EVT_SEND_TIME) {
			SC_PACKET_TIME packets;
			packets.size = sizeof(SC_PACKET_TIME);
			packets.type = SC_WORLD_TIME;
			packets.world_time = overlapped_buffer->world_time;
			//printf("시간 보냄 %f \n", packets.world_time);

			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (m_Clients[i].m_bUsing)
					SendPacket(i, &packets);
			}
		}
		else if (overlapped_buffer->evt_type == EVT_BULLET_UPDATE) {
			// i 가 플레이어
			// j 가 플레이어가 발사한 총알
			for (int i = 0; i < MAX_PLAYER; ++i) {
				for (int j = 1; j <= MAX_AMMO; ++j) {
					if (m_Clients[i].m_bUsing) {
						m_mutexBulletLock[i].lock();
						if (bullets[i][j].m_bUsing) {
							bullets[i][j].m_fBallisticsTime += overlapped_buffer->elapsed_time;
							bullets[i][j].x += (-1) * METER_PER_PIXEL * bullets[i][j].m_v3LookVec.x * (AR_SPEED * overlapped_buffer->elapsed_time);
							bullets[i][j].y += (-1) * METER_PER_PIXEL * bullets[i][j].m_v3LookVec.y * (AR_SPEED * overlapped_buffer->elapsed_time);
							bullets[i][j].y -= (9.8 / 2.f) * bullets[i][j].m_fBallisticsTime * bullets[i][j].m_fBallisticsTime / (AR_SPEED * 2);
							bullets[i][j].z += (-1) * METER_PER_PIXEL * bullets[i][j].m_v3LookVec.z * (AR_SPEED * overlapped_buffer->elapsed_time);

							if (bullets[i][j].y <= 0.f) {
								bullets[i][j].m_bUsing = false;
							}
							else if (bullets[i][j].x >= 265.f || bullets[i][j].x <= -265.f) {
								bullets[i][j].m_bUsing = false;
							}
							else if (bullets[i][j].z >= 265.f || bullets[i][j].z <= -265.f) {
								bullets[i][j].m_bUsing = false;
							}
							
							SC_PACKET_BULLET packets;
							packets.id = i;
							packets.size = sizeof(SC_PACKET_BULLET);
							packets.type = SC_BULLET_POS;
							packets.bullet_id = j;
							packets.m_bInUse = bullets[i][j].m_bUsing;
							packets.x = bullets[i][j].x;
							packets.y = bullets[i][j].y;
							packets.z = bullets[i][j].z;
							// 해당 플레이어에게만 보내야함
							for (int k = 0; k < MAX_PLAYER; ++k) {
								if (m_Clients[k].m_bUsing)
									SendPacket(k, &packets);
							}

						}
						m_mutexBulletLock[i].unlock();
					}
				}
			}

		}
		// Send로 인해 할당된 영역 반납
		else if (overlapped_buffer->evt_type == EVT_SEND_PACKET) {
			delete overlapped_buffer;
		}
	}
}

void ServerFramework::SendPacket(int cl_id, void* packet) {
	OverlappedExtensionSet* overlapped = new OverlappedExtensionSet;
	char* send_buffer = reinterpret_cast<char*>(packet);
	//printf("[보냄] %d Pakcet을 %d\n", send_buffer[1], cl_id);
	memcpy(&overlapped->io_buffer, packet, send_buffer[0]);
	overlapped->evt_type = EVT_SEND_PACKET;
	overlapped->wsabuf.buf = overlapped->io_buffer;
	overlapped->wsabuf.len = send_buffer[0];
	ZeroMemory(&overlapped->wsa_over, sizeof(WSAOVERLAPPED));
	unsigned long flag = 0;
	int retval = WSASend(m_Clients[cl_id].s, &overlapped->wsabuf, 1, NULL, 0,
		&overlapped->wsa_over, NULL);

	if (retval != 0) {
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING) {
			ErrorDisplay("SendPacket에서 에러 발생 : ", err_no);
		}
	}
}

void ServerFramework::DisconnectPlayer(int cl_id) {
	// 플레이어 접속 끊기
	closesocket(m_Clients[cl_id].s);
	m_Clients[cl_id].m_bUsing = false;
	printf("[DisconnectPlayer] ClientID : %d\n", cl_id);
	SC_PACKET_REMOVE_PLAYER packet;
	packet.client_id = cl_id;
	packet.size = sizeof(SC_PACKET_REMOVE_PLAYER);
	packet.type = SC_REMOVE_PLAYER;

	// 플레이어가 나갔다는 정보를 모든 클라이언트에 뿌려준다.
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (m_Clients[i].m_bUsing == true) {
			SendPacket(i, &packet);
		}
	}

}

void ServerFramework::Update(duration<float>& elapsed_time) {

	//ol_ex[4].evt_type = EVT_PLAYER_POS_UPDATE;
	//ol_ex[4].elapsed_time = elapsed_time.count();
	//PostQueuedCompletionStatus(iocp_handle, 0, 4, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[4]));
	if (m_bGameStart) {
		ol_ex[5].evt_type = EVT_COLLISION;
		PostQueuedCompletionStatus(iocp_handle, 0, 5, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[5]));

		ol_ex[7].evt_type = EVT_BULLET_UPDATE;
		ol_ex[7].elapsed_time = elapsed_time.count();
		PostQueuedCompletionStatus(iocp_handle, 0, 7, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[7]));

		m_fTimeSend += elapsed_time.count();
		if (m_fTimeSend >= TIME_SEND_TIME) {
			ol_ex[9].evt_type = EVT_SEND_TIME;
			ol_ex[9].world_time = ITEM_BOAT_GEN_TIME - m_fBoatGenTime;
			PostQueuedCompletionStatus(iocp_handle, 0, 9, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[9]));
			m_fTimeSend = 0.f;
		}

		if (m_bIsBoatGen) {
			m_fBoatGenTime += elapsed_time.count();
			if (m_fBoatGenTime >= ITEM_BOAT_GEN_TIME) {
				ol_ex[8].evt_type = EVT_BOAT_ITEM_GEN;
				PostQueuedCompletionStatus(iocp_handle, 0, 0, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[8]));
				m_fBoatGenTime = 0.f;
				//m_bIsBoatGen = false;
			}
		}

		if (m_bIsAmmoGen) {
			m_fAmmoGenTime += elapsed_time.count();
			if (m_fAmmoGenTime >= ITEM_AMMO_GEN_TIME) {
				ol_ex[10].evt_type = EVT_AMMO_ITEM_GEN;
				PostQueuedCompletionStatus(iocp_handle, 0, 0, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[10]));
				m_fAmmoGenTime = 0.f;
				//m_bIsAmmoGen = false;
			}
		}

		// 1초마다 플레이어 체력 갱신
		m_fPlayerHpUpdateTime += elapsed_time.count();
		if (m_fPlayerHpUpdateTime >= PLAYER_HP_UPDATE_TIME) {
			ol_ex[11].evt_type = EVT_PLAYER_HP_UPDATE;
			PostQueuedCompletionStatus(iocp_handle, 0, 11, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[11]));
			m_fPlayerHpUpdateTime = 0.f;
		}


		sender_time += elapsed_time.count();
		if (sender_time >= UPDATE_TIME) {   // 1/60 초마다 데이터 송신
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (m_Clients[i].m_bMoveBackward || m_Clients[i].m_bMoveFoward || m_Clients[i].m_bMoveLeft || m_Clients[i].m_bMoveRight) {
					ol_ex[i].evt_type = EVT_PLAYER_POS_SEND;
					ol_ex[i].elapsed_time = elapsed_time.count() + sender_time;
					PostQueuedCompletionStatus(iocp_handle, 0, i, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[i]));
				}
			}
			sender_time = 0.f;
		}


	}




}