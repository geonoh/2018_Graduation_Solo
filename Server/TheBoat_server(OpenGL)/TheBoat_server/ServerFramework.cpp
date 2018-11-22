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
		clients[i].x = 0.f;
		clients[i].z = 0.f;
		clients[i].y = height_map->GetHeight(clients[i].x + DX12_TO_OPGL, clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
		clients[i].hp = 100.f;
		clients[i].stamina = 100.f;
		clients[i].ammo_current = 30;
		clients[i].ammo_total = 90;
		for (int j = 0; j < 4; ++j) {
			clients[i].gathered_parts[j] = false;
		}
	}
#ifdef _Dev
	printf("---------------------------------\n");
	printf("- 개발모드\n");
	printf("---------------------------------\n");
	is_boat_item_gen = true;
	game_start = true;
	ammo_gen = true;
	time_boat = 0.f;
	for (int i = 0; i < MAX_PLAYER; ++i) {
		clients[i].x = 158.f;
		clients[i].y = 45.f;
		clients[i].z = -183.f;
		clients[i].stamina = 100.f;
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
		if (clients[i].is_use == false) {
			client_id = i;
			break;
		}
	}
	if (client_id == -1) {
		printf("최대 유저 초과\n");
	}
	printf("[%d] 플레이어 입장\n", client_id);
	clients[client_id].s = client_socket;

	clients[client_id].is_ready = false;
	clients[client_id].is_running = false;
	ZeroMemory(&clients[client_id].overlapped_ex.wsa_over, sizeof(WSAOVERLAPPED));
	//clients[client_id].overlapped_ex.is_recv = true;
	clients[client_id].overlapped_ex.evt_type = EVT_RECV_PACKET;
	clients[client_id].overlapped_ex.wsabuf.buf = clients[client_id].overlapped_ex.io_buffer;
	clients[client_id].overlapped_ex.wsabuf.len = sizeof(clients[client_id].overlapped_ex.io_buffer);
	clients[client_id].packet_size = 0;
	clients[client_id].prev_packet_size = 0;



	//clients[client_id].team = e_TeamRed;
	clients[client_id].team = e_NoTeam;


	// 플레이어 입장 표시
	clients[client_id].is_use = true;

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(client_socket),
		iocp_handle, client_id, 0);
	unsigned long flag = 0;
	WSARecv(client_socket, &clients[client_id].overlapped_ex.wsabuf, 1, NULL,
		&flag, &clients[client_id].overlapped_ex.wsa_over, NULL);

	// 플레이어 입장했다고 패킷 보내줘야함.
	// 이 정보에는 플레이어의 초기 위치정보도 포함되어야 한다. 
	SC_PACKET_ENTER_PLAYER packet;
	packet.id = client_id;
	packet.size = sizeof(SC_PACKET_ENTER_PLAYER);
	packet.type = SC_ENTER_PLAYER;
	packet.hp = clients[client_id].hp;
	packet.stamina = clients[client_id].stamina;
	packet.x = clients[client_id].x;
	packet.y = clients[client_id].y;
	packet.z = clients[client_id].z;

	lock_ammo[client_id].lock();
	packet.ammo_total = clients[client_id].ammo_total;
	packet.ammo_current = clients[client_id].ammo_current;
	lock_ammo[client_id].unlock();

	SendPacket(client_id, &packet);

	// 나 제외 플레이어에게 입장정보 전송
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (clients[i].is_use && (client_id != i)) {
			//printf("%d에게 %d의 정보를 보낸다\n", i, client_id);
			SendPacket(i, &packet);
		}
	}


	// 다른 클라이언트의 위치 보내주기
	for (int i = 0; i < MAX_PLAYER; ++i) {
		ZeroMemory(&packet, sizeof(packet));
		if (i != client_id) {
			if (clients[i].is_use == true) {
				packet.id = i;
				packet.size = sizeof(SC_PACKET_ENTER_PLAYER);
				packet.type = SC_ENTER_PLAYER;
				clients[i].y = height_map->GetHeight(clients[i].x + DX12_TO_OPGL, clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
				packet.x = clients[i].x;
				packet.y = clients[i].y;
				packet.z = clients[i].z;
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
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		clients[cl_id].move_foward = true;
		break;
	case CS_KEY_PRESS_DOWN:
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		clients[cl_id].move_backward = true;
		break;
	case CS_KEY_PRESS_LEFT:
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		clients[cl_id].move_left = true;
		break;
	case CS_KEY_PRESS_RIGHT:
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		clients[cl_id].move_right = true;
		break;
		// 디버그 용임-> 시간 빨리 깎기
	case CS_DEBUG_TIME:
		time_boat += 10;

		break;
	case CS_KEY_PRESS_1:
		clients[cl_id].weapon = 0;
		break;
	case CS_KEY_PRESS_2:
		clients[cl_id].weapon = 1;
		break;

	case CS_KEY_PRESS_SHIFT:
		clients[cl_id].is_running = true;
		break;
	case CS_KEY_PRESS_SPACE:
		break;

	case CS_KEY_RELEASE_UP:
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		clients[cl_id].move_foward = false;
		break;
	case CS_KEY_RELEASE_DOWN:
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		clients[cl_id].move_backward = false;
		break;
	case CS_KEY_RELEASE_LEFT:
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		clients[cl_id].move_left = false;
		break;
	case CS_KEY_RELEASE_RIGHT:
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		clients[cl_id].move_right = false;
		break;
	case CS_KEY_RELEASE_1:
		break;
	case CS_KEY_RELEASE_2:
		break;
	case CS_KEY_RELEASE_SHIFT:
		clients[cl_id].is_running = false;
		break;
	case CS_KEY_RELEASE_SPACE:
		break;

	case CS_RIGHT_BUTTON_DOWN:
		clients[cl_id].click_right = true;
		break;
	case CS_RIGHT_BUTTON_UP:
		clients[cl_id].click_right = false;
		break;
	case CS_RELOAD:
		// AR 장착중인경우
		if (clients[cl_id].weapon == 0) {
			// 장전하려고 보니 결과가 0 이상
			lock_ammo[cl_id].lock();
			if (clients[cl_id].ammo_total > 0) {
				if (clients[cl_id].ammo_total - clients[cl_id].ammo_current > 0) {
					clients[cl_id].ammo_total -= (30 - clients[cl_id].ammo_current);
					clients[cl_id].ammo_current = 30;
					SC_PACKET_AMMO_O packets;
					packets.size = sizeof(SC_PACKET_AMMO_O);
					packets.type = SC_FULLY_AMMO;
					packets.ammo = clients[cl_id].ammo_current;
					packets.m_cTotalAmmo = clients[cl_id].ammo_total;
					SendPacket(cl_id, &packets);
				}
				else if (clients[cl_id].ammo_total - clients[cl_id].ammo_current <= 0) {
					clients[cl_id].ammo_current += clients[cl_id].ammo_total;
					clients[cl_id].ammo_total = 0;
					SC_PACKET_AMMO_O packets;
					packets.size = sizeof(SC_PACKET_AMMO_O);
					packets.type = SC_FULLY_AMMO;
					packets.ammo = clients[cl_id].ammo_current;
					SendPacket(cl_id, &packets);
				}
			}
			lock_ammo[cl_id].unlock();
		}
		// SUB 장착중인경우
		else {
			// --------------------------------------------------------------
			// 2018 07 31 : 해야해요! 보조무기
			// --------------------------------------------------------------

			//// 장전하려고 보니 결과가 0 이상
			//if (clients[cl_id].sub_ammo - bullet_counter[cl_id] > 0) {
			//	clients[cl_id].sub_ammo -= bullet_counter[cl_id];
			//	bullet_counter[cl_id] = 0;
			//	printf("장전 완료, 남은 탄창 %d \n", clients[cl_id].sub_ammo);
			//	SC_PACKET_AMMO_O packets;
			//	packets.size = sizeof(SC_PACKET_AMMO_O);
			//	packets.type = SC_FULLY_AMMO;
			//	packets.ammo = bullet_counter[cl_id];
			//	SendPacket(cl_id, &packets);
			//}
			//// 장전하려고 보니 결과가 0 미만
			//else {
			//	bullet_counter[cl_id] -= clients[cl_id].sub_ammo;
			//	clients[cl_id].sub_ammo = 0;
			//	printf("장전 완료, 남은 탄창 %d \n", clients[cl_id].sub_ammo);
			//	SC_PACKET_AMMO_O packets;
			//	packets.size = sizeof(SC_PACKET_AMMO_O);
			//	packets.type = SC_FULLY_AMMO;
			//	packets.ammo = bullet_counter[cl_id];
			//	SendPacket(cl_id, &packets);

			//}

		}
		break;
	case CS_LEFT_BUTTON_DOWN:
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		if (clients[cl_id].weapon == 0) {
			if (clients[cl_id].ammo_current == 0) {
				SC_PACKET_AMMO_O packets;
				packets.size = sizeof(SC_PACKET_AMMO_O);
				packets.type = SC_OUT_OF_AMMO;
				SendPacket(cl_id, &packets);
			}
			else {
				clients[cl_id].click_left = true;
				ol_ex[6].evt_type = EVT_BULLET_GENERATE;
				ol_ex[6].shooter_player_id = cl_id;
				//ol_ex[6].elapsed_time = elapsed_time.count();
				PostQueuedCompletionStatus(iocp_handle, 0, 6, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[6]));
			}
		}
		// 보조무기
		else if (clients[cl_id].weapon == 1) {
			//if (bullet_counter[cl_id] == MAX_AMMO) {
			//	printf("총알 장전 필요\n");
			//	SC_PACKET_AMMO_O packets;
			//	packets.size = sizeof(SC_PACKET_AMMO_O);
			//	packets.type = SC_OUT_OF_AMMO;
			//	SendPacket(cl_id, &packets);
			//}
			//else {
			//	clients[cl_id].click_left = true;
			//	ol_ex[6].evt_type = EVT_BULLET_GENERATE;
			//	ol_ex[6].shooter_player_id = cl_id;
			//	//ol_ex[6].elapsed_time = elapsed_time.count();
			//	PostQueuedCompletionStatus(iocp_handle, 0, 6, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[6]));
			//}
		}

		break;
	case CS_LEFT_BUTTON_UP: {
		clients[cl_id].click_left = false;
		break;
	}
	case CS_MOUSE_MOVE: {
		clients[cl_id].look_vector.x = packet_buffer->look_vector.x;
		clients[cl_id].look_vector.y = packet_buffer->look_vector.y;
		clients[cl_id].look_vector.z = packet_buffer->look_vector.z;
		SC_PACKET_LOOCVEC packets;
		packets.id = cl_id;
		packets.size = sizeof(SC_PACKET_LOOCVEC);
		packets.type = SC_PLAYER_LOOKVEC;
		packets.look_vector.x = clients[cl_id].look_vector.x;
		packets.look_vector.y = clients[cl_id].look_vector.y;
		packets.look_vector.z = clients[cl_id].look_vector.z;
		// 플레이어가 뒤는 상황

		//if (clients[cl_id].click_left) {
		//	packets.player_status = 3;
		//}
		//else if (clients[cl_id].is_running) {
		//	packets.player_status = 2;
		//}
		// 걷지도 뛰지도 않는 상황


		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].is_use == true) {
				SendPacket(i, &packets);
			}
		}
		break;
	}
	case CS_PLAYER_READY: {
		int ready_count = 0;
		clients[cl_id].is_ready = true;
		//if (clients[0].is_ready && clients[1].is_ready && clients[2].is_ready && clients[3].is_ready) {
		//	GameStart();
		//}
		int iReadyCounter = 0;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].is_use && clients[i].is_ready) {
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
			if (clients[i].is_use)
				SendPacket(i, &packets);
		}

		break;
	}
	case CS_PLAYER_READY_CANCLE: {
		clients[cl_id].is_ready = false;
		SC_PACKET_READY packets;
		packets.size = sizeof(SC_PACKET_READY);
		packets.type = SC_PLAYER_READY_CANCLE;
		packets.m_cPlayerNumber = cl_id;

		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].is_use)
				SendPacket(i, &packets);
		}

		break;
	}
	case CS_TEAM_RED: {
		clients[cl_id].team = e_TeamRed;
		CS_PACKET_TEAM_SELECT packets;
		packets.size = sizeof(CS_PACKET_TEAM_SELECT);
		packets.type = SC_TEAM_RED;
		packets.id = cl_id;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].is_use)
				SendPacket(i, &packets);
		}
		break;
	}
	case CS_TEAM_BLUE: {
		clients[cl_id].team = e_TeamBlue;
		CS_PACKET_TEAM_SELECT packets;
		packets.size = sizeof(CS_PACKET_TEAM_SELECT);
		packets.type = SC_TEAM_BLUE;
		packets.id = cl_id;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].is_use)
				SendPacket(i, &packets);
		}
		break;
	}
	case CS_MODE_TEAM: {
		SC_PACKET_GAMEMODE packets;
		packets.size = sizeof(SC_PACKET_GAMEMODE);
		packets.type = SC_MODE_TEAM;
		game_mode = false;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].is_use) {
				SendPacket(i, &packets);
			}
		}
		break;
	}
	case CS_MODE_MELEE: {
		SC_PACKET_GAMEMODE packets;
		packets.size = sizeof(SC_PACKET_GAMEMODE);
		packets.type = SC_MODE_MELEE;
		game_mode = true;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].is_use) {
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
			clients[i].x = 0.f;
			clients[i].z = 0.f;
			clients[i].y = height_map->GetHeight(clients[i].x + DX12_TO_OPGL, clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
			clients[i].hp = 100.f;
			clients[i].stamina = 100.f;
			clients[i].ammo_current = 30;
			clients[i].ammo_total = 90;
			clients[i].is_ready = false;
			clients[i].team = e_NoTeam;
			is_die_sended[i] = false;
			for (int j = 0; j < 4; ++j) {
				clients[i].gathered_parts[j] = false;
			}
		}
		is_boat_item_gen = false;
		game_start = false;
		ammo_gen = false;
		time_boat = 0.f;

		break;
	}
	case CS_ASSENBLE_PARTS: {
		//printf("조립 가즈아\n");
		// 게임 Status Update
		// 이걸 F키 눌렀을때 해야함 특정지역에서 
		if (game_mode == false) {
			int iTeamRed = 0;
			int iTeamBlue = 0;
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (clients[i].team == e_TeamRed) {
					for (int j = 0; j < 4; ++j) {
						if (clients[i].gathered_parts[j]) {
							iTeamRed++;
						}
					}
				}
				else if (clients[i].team == e_TeamBlue) {
					for (int j = 0; j < 4; ++j) {
						if (clients[i].gathered_parts[j]) {
							iTeamBlue++;
						}
					}
				}
			}
			// 보트 부품 다 모은경우
			// Red 승리
			if (iTeamRed == 4) {
				//printf("레드 승리 \n");
				//game_start = false;
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
				//game_start = false;
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
					if (clients[i].gathered_parts[j]) {
						iPlayerCounter[i]++;
					}
				}

				if (iPlayerCounter[i] == 4) {
					//printf("%d 플레이어 승리\n", i + 1);
					//game_start = false;
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
		packets.m_bPlayerTeam[i] = clients[i].team;
	}
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (clients[i].is_use)
			SendPacket(i, &packets);
	}

	clients[0].x = -195.f;
	clients[0].z = -120.f;
	clients[0].y = height_map->GetHeight(clients[0].x + DX12_TO_OPGL, clients[0].z + DX12_TO_OPGL) + PLAYER_HEIGHT;

	clients[1].x = -130.f;
	clients[1].z = 138.f;
	clients[1].y = height_map->GetHeight(clients[1].x + DX12_TO_OPGL, clients[1].z + DX12_TO_OPGL) + PLAYER_HEIGHT;

	clients[2].x = 128.f;
	clients[2].z = 128.f;
	clients[2].y = height_map->GetHeight(clients[2].x + DX12_TO_OPGL, clients[2].z + DX12_TO_OPGL) + PLAYER_HEIGHT;

	clients[3].x = 117.f;
	clients[3].z = -136.f;
	clients[3].y = height_map->GetHeight(clients[3].x + DX12_TO_OPGL, clients[3].z + DX12_TO_OPGL) + PLAYER_HEIGHT;

	for (int i = 0; i < MAX_PLAYER; ++i) {
		clients[i].stamina = 100.f;
		ol_ex[i].evt_type = EVT_PLAYER_POS_SEND;
		PostQueuedCompletionStatus(iocp_handle, 0, i, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[i]));
	}
	for (int i = 0; i < MAX_PLAYER; ++i) {
		for (int j = 1; j <= MAX_AMMO; ++j) {
			bullets[i][j].is_use = false;
		}
	}

	is_boat_item_gen = true;
	game_start = true;
	ammo_gen = true;
	time_boat = 0.f;
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
				if (clients[client_id].packet_size == 0) {
					clients[client_id].packet_size = ptr[0];
				}
				int remain = clients[client_id].packet_size - clients[client_id].prev_packet_size;
				if (remain <= recved_size) {
					memcpy(clients[client_id].prev_packet + clients[client_id].prev_packet_size,
						ptr,
						remain);
					ProcessPacket(static_cast<int>(client_id), clients[client_id].prev_packet);
					recved_size -= remain;
					ptr += remain;
					clients[client_id].packet_size = 0;
					clients[client_id].prev_packet_size = 0;
				}
				else {
					memcpy(clients[client_id].prev_packet + clients[client_id].prev_packet_size,
						ptr,
						recved_size);
					recved_size -= recved_size;
					ptr += recved_size;
				}
			}

			unsigned long rflag = 0;
			ZeroMemory(&overlapped_buffer->wsa_over, sizeof(WSAOVERLAPPED));
			int retval = WSARecv(clients[client_id].s, &overlapped_buffer->wsabuf, 1, NULL, &rflag, &overlapped_buffer->wsa_over, NULL);
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
			if (game_mode) {
				for (int i = 0; i < MAX_PLAYER; ++i) {
					if (clients[i].hp <= 0.f) {
						iAliveCounter--;
					}
				}
				
				if (iAliveCounter == 1) {
					//game_start = false;
					for (int j = 0; j < MAX_PLAYER; ++j) {
						// 체력이 양수인 플레이어가 최종 생존자라는 뜻
						if (clients[j].hp > 0.f) {
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
					if (clients[j].hp <= 0.f && clients[j].team == e_TeamRed) {
						iDeathCountRed++;
					}
					if (clients[j].hp <= 0.f && clients[j].team == e_TeamBlue) {
						iDeathCountBlue++;
					}
				}
				//iAliveCounterRed--;
				if (iDeathCountRed == MAX_PLAYER / 2) {
					//game_start = false;
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
					//game_start = false;
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
				if (clients[i].hp <= 0.f && is_die_sended[i] == false) {
					//printf("%d 플레이어 Die\n",i);
					SC_PACKET_DIE packets;
					packets.size = sizeof(SC_PACKET_DIE);
					packets.type = SC_PLAYER_DIE;
					packets.who_die = i;

					// 죽은 플레이어가 아이템을 가지고 있으면 해당위치에 Drop
					for (int j = 0; j < 4; ++j) {
						// 만약 가지고 있다면 
						if (clients[i].gathered_parts[j] == true) {
							item_boat[j].is_use = true;
							item_boat[j].x = clients[i].x;
							item_boat[j].y = clients[i].y;
							item_boat[j].z = clients[i].z;
							clients[i].gathered_parts[j] = false;
							packets.m_bBoatItem[j] = true;
							printf("죽고 %d 아이템 드랍\n", j);
						}
						//
					}

					packets.die_pos_x = clients[i].x;
					packets.die_pos_y = clients[i].y;
					packets.die_pos_z = clients[i].z;
					for (int j = 0; j < MAX_PLAYER; ++j) {
						SendPacket(j, &packets);
					}
					is_die_sended[i] = true;
				}
				else {
					// 플레이어가 살아있으면 Stamina 회복해줌.
					if (clients[i].move_backward == false &&
						clients[i].move_foward == false &&
						clients[i].move_left == false &&
						clients[i].move_right == false &&
						clients[i].stamina < 95.f) {
						clients[i].stamina += 5.f;
						SC_PACKET_STAMINA packets;
						packets.size = sizeof(SC_PACKET_STAMINA);
						packets.type = SC_STAMINA;
						packets.stamina = clients[i].stamina;
						packets.id = i;
						SendPacket(i, &packets);
					}

				}
				if (clients[i].is_use && map_boat_gen[0]) {
					if (-256.f < clients[i].x&&clients[i].x <= 0.f) {
						if (-256.f <= clients[i].z && clients[i].z <= 0.f) {
							clients[i].hp -= 1.f;
							SC_PACKET_PLAYER_HP_UPDATE packets;
							packets.size = sizeof(SC_PACKET_PLAYER_HP_UPDATE);
							packets.type = SC_PLAYER_HP_UPDATE;
							packets.hp = clients[i].hp;
							packets.m_cPlayerID = i;
							printf("%d Player HP : %f \n", packets.m_cPlayerID, packets.hp);
							SendPacket(i, &packets);
						}
					}
				}
				if (clients[i].is_use && map_boat_gen[1]) {
					if (-256.f < clients[i].x&&clients[i].x <= 0) {
						if (0 <= clients[i].z && clients[i].z <= 256.f) {
							clients[i].hp -= 1.f;
							SC_PACKET_PLAYER_HP_UPDATE packets;
							packets.size = sizeof(SC_PACKET_PLAYER_HP_UPDATE);
							packets.type = SC_PLAYER_HP_UPDATE;
							packets.hp = clients[i].hp;
							packets.m_cPlayerID = i;
							printf("%d Player HP : %f \n", packets.m_cPlayerID, packets.hp);
							SendPacket(i, &packets);
						}
					}
				}
				if (clients[i].is_use && map_boat_gen[2]) {
					if (0.f < clients[i].x&&clients[i].x <= 256.f) {
						if (0 <= clients[i].z && clients[i].z <= 256.f) {
							clients[i].hp -= 1.f;
							SC_PACKET_PLAYER_HP_UPDATE packets;
							packets.size = sizeof(SC_PACKET_PLAYER_HP_UPDATE);
							packets.type = SC_PLAYER_HP_UPDATE;
							packets.hp = clients[i].hp;
							packets.m_cPlayerID = i;
							printf("%d Player HP : %f \n", packets.m_cPlayerID, packets.hp);
							SendPacket(i, &packets);
						}
					}
				}
				if (clients[i].is_use && map_boat_gen[3]) {
					if (0.f < clients[i].x&&clients[i].x <= 256.f) {
						if (-256.f <= clients[i].z && clients[i].z <= 0.f) {
							clients[i].hp -= 1.f;
							SC_PACKET_PLAYER_HP_UPDATE packets;
							packets.size = sizeof(SC_PACKET_PLAYER_HP_UPDATE);
							packets.type = SC_PLAYER_HP_UPDATE;
							packets.hp = clients[i].hp;
							packets.m_cPlayerID = i;
							printf("%d Player HP : %f \n", packets.m_cPlayerID, packets.hp);
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
				item_ammo[0].is_use = true;
				item_ammo[0].type_item = 4;
				item_ammo[0].x = -204.f;
				item_ammo[0].y = 79.f;
				item_ammo[0].z = -186.f;
				break;
			case 1:
				item_ammo[1].is_use = true;
				item_ammo[1].type_item = 4;
				item_ammo[1].x = -195.f;
				item_ammo[1].y = 72.f;
				item_ammo[1].z = -73.f;
				break;
			case 2:
				item_ammo[2].is_use = true;
				item_ammo[2].type_item = 4;
				item_ammo[2].x = -160.f;
				item_ammo[2].y = 81.f;
				item_ammo[2].z = 71.f;
				break;
			case 3:
				item_ammo[3].is_use = true;
				item_ammo[3].type_item = 4;
				item_ammo[3].x = -167.f;
				item_ammo[3].y = 45.f;
				item_ammo[3].z = 201.f;
				break;
			case 4:
				item_ammo[4].is_use = true;
				item_ammo[4].type_item = 4;
				item_ammo[4].x = 79.f;
				item_ammo[4].y = 43.f;
				item_ammo[4].z = 191.f;
				break;
			case 5:
				item_ammo[5].is_use = true;
				item_ammo[5].type_item = 4;
				item_ammo[5].x = 162.f;
				item_ammo[5].y = 61.f;
				item_ammo[5].z = 95.f;
				break;
			case 6:
				item_ammo[6].is_use = true;
				item_ammo[6].type_item = 4;
				item_ammo[6].x = 170.f;
				item_ammo[6].y = 57.f;
				item_ammo[6].z = -58.f;
				break;
			case 7:
				item_ammo[7].is_use = true;
				item_ammo[7].type_item = 4;
				item_ammo[7].x = 158.f;
				item_ammo[7].y = 45.f;
				item_ammo[7].z = -183.f;
				break;
			}
			printf("Ammo : No.%d 생성 [%f, %f, %f]\n", iDice,
				item_ammo[iDice].x,
				item_ammo[iDice].y,
				item_ammo[iDice].z);

			SC_PACKET_ITEM_GEN packets;
			packets.size = sizeof(SC_PACKET_ITEM_GEN);
			packets.type = SC_AMMO_ITEM_GEN;
			packets.item_type = 4;
			packets.m_cItemID = iDice;
			packets.x = item_ammo[iDice].x;
			packets.y = item_ammo[iDice].y;
			packets.z = item_ammo[iDice].z;
			for (int i = 0; i < MAX_PLAYER; ++i) {
				SendPacket(i, &packets);
			}

		}
		else if (overlapped_buffer->evt_type == EVT_is_boat_item_gen) {

			if (count_dice < 4) {
				lock_boat_item.lock();
				int iDice = 0;
				while (true) {
					iDice = rand() % 4;
					// 랜덤하게 접근한 번호가 사용중이면  닫시 랜덤 
					if (map_boat_gen[iDice] == true) {
						continue;
					}
					else {
						count_dice++;
						item_boat[iDice].is_use = true;
						item_boat[iDice].type_item = iDice;
						map_boat_gen[iDice] = true;

						break;
					}
				}
				switch (iDice) {
				case 0:
					item_boat[iDice].x = -185.f;
					item_boat[iDice].y = 93.f;
					item_boat[iDice].z = -208.f;
					break;
				case 1:
					item_boat[iDice].x = -110.f;
					item_boat[iDice].y = 71.f;
					item_boat[iDice].z = 138.f;
					break;
				case 2:
					item_boat[iDice].x = 113.f;
					item_boat[iDice].y = 89.f;
					item_boat[iDice].z = 160.f;
					break;
				case 3:
					item_boat[iDice].x = 81;
					item_boat[iDice].y = 79.f;
					item_boat[iDice].z = -129.f;
					break;
				}

				SC_PACKET_ITEM_GEN packets;
				packets.size = sizeof(SC_PACKET_ITEM_GEN);
				packets.type = SC_is_boat_item_gen;
				packets.item_type = item_boat[iDice].type_item;
				packets.x = item_boat[iDice].x;
				packets.y = item_boat[iDice].y;
				packets.z = item_boat[iDice].z;
				printf("%d Type 보냈다\n", packets.item_type);
				for (int i = 0; i < MAX_PLAYER; ++i) {
					SendPacket(i, &packets);
				}
				lock_boat_item.unlock();
			}
			if (count_dice == 2) {
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
				clients[i].lock_server.lock();
				if (clients[i].move_foward) {
					if (clients[i].is_running && clients[i].stamina > 0.f) {
						clients[i].stamina -= 0.1f;
						if (31.f <= clients[i].x &&clients[i].x < 34.f && -92.f <= clients[i].z && clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && 26.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -153.f && -92.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -92.f <= clients[i].z && clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= clients[i].x && clients[i].x <= 33.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -180.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -154.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (clients[i].x > 255.f) {
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vector.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vector.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
					}
					// 걷기 
					else {
						if (31.f <= clients[i].x &&clients[i].x < 34.f && -92.f <= clients[i].z && clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && 26.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -153.f && -92.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -92.f <= clients[i].z && clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= clients[i].x && clients[i].x <= 33.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -180.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -154.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (clients[i].x > 255.f) {
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vector.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vector.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}

					}
				}
				if (clients[i].move_backward) {

					if (clients[i].is_running && clients[i].stamina > 0.f) {
						clients[i].z += METER_PER_PIXEL * clients[i].look_vector.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += METER_PER_PIXEL * clients[i].look_vector.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].stamina -= 0.1f;

						//if (31.f <= clients[i].x &&clients[i].x < 34.f && -92.f <= clients[i].z && clients[i].z <= 28) {
						//	//printf("1번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= 33.f && 26.f <= clients[i].z && clients[i].z <= 28.f) {
						//	//printf("2번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f,0.f,1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= -153.f && -92.f <= clients[i].z && clients[i].z <= 28.f) {
						//	//printf("3번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { -1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -92.f <= clients[i].z && clients[i].z <= -89.f) {
						//	//printf("4번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f,0.f, -1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (30.f <= clients[i].x && clients[i].x <= 33.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
						//	//printf("5번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -180.f <= clients[i].z && clients[i].z <= -178.f) {
						//	//printf("6번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= -154.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
						//	//printf("7번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (clients[i].x > 255.f) {
						//	printf("빠꾸치다가 넘었다\n");
						//	glm::vec3 v3Normal = { -1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-255.f > clients[i].x) {
						//	glm::vec3 v3Normal = { 1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-255.f > clients[i].z) {
						//	glm::vec3 v3Normal = { 0.f,0.f,1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (255.f < clients[i].z) {
						//	glm::vec3 v3Normal = { 0.f,0.f,-1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else {
						//	clients[i].z -= (-1) * METER_PER_PIXEL * clients[i].look_vector.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= (-1) * METER_PER_PIXEL * clients[i].look_vector.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//}

					}
					// 걷기
					else {
						clients[i].z += METER_PER_PIXEL * clients[i].look_vector.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += METER_PER_PIXEL * clients[i].look_vector.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//if (31.f <= clients[i].x &&clients[i].x < 34.f && -92.f <= clients[i].z && clients[i].z <= 28) {
						//	//printf("1번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= 33.f && 26.f <= clients[i].z && clients[i].z <= 28.f) {
						//	//printf("2번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f,0.f,1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= -153.f && -92.f <= clients[i].z && clients[i].z <= 28.f) {
						//	//printf("3번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { -1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -92.f <= clients[i].z && clients[i].z <= -89.f) {
						//	//printf("4번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f,0.f, -1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (30.f <= clients[i].x && clients[i].x <= 33.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
						//	//printf("5번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -180.f <= clients[i].z && clients[i].z <= -178.f) {
						//	//printf("6번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-156.f <= clients[i].x && clients[i].x <= -154.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
						//	//printf("7번 벽 부닺침\n");
						//	glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (clients[i].x > 255.f) {
						//	glm::vec3 v3Normal = { -1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-255.f > clients[i].x) {
						//	glm::vec3 v3Normal = { 1.f,0.f,0.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (-255.f > clients[i].z) {
						//	glm::vec3 v3Normal = { 0.f,0.f,1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else if (255.f < clients[i].z) {
						//	glm::vec3 v3Normal = { 0.f,0.f,-1.f };
						//	glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
						//	clients[i].z -= -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}
						//else {
						//	clients[i].z -= (-1) * METER_PER_PIXEL * clients[i].look_vector.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//	clients[i].x -= (-1) * METER_PER_PIXEL * clients[i].look_vector.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//}

					}

				}
				if (clients[i].move_left) {
					if (clients[i].is_running && clients[i].stamina > 0.f) {
						//clients[i].z += METER_PER_PIXEL * clients[i].look_vector.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vector.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].stamina -= 0.1f;

						if (31.f <= clients[i].x &&clients[i].x < 34.f && -92.f <= clients[i].z && clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && 26.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -153.f && -92.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -92.f <= clients[i].z && clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= clients[i].x && clients[i].x <= 33.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -180.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -154.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (clients[i].x > 255.f) {
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							clients[i].z += METER_PER_PIXEL * clients[i].look_vector.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vector.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}

					}
					else {
						//clients[i].z += METER_PER_PIXEL * clients[i].look_vector.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vector.z * (WALK_SPEED * overlapped_buffer->elapsed_time);

						if (31.f <= clients[i].x &&clients[i].x < 34.f && -92.f <= clients[i].z && clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && 26.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -153.f && -92.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -92.f <= clients[i].z && clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= clients[i].x && clients[i].x <= 33.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -180.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -154.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (clients[i].x > 255.f) {
							printf("빠꾸치다가 넘었다\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += -METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							clients[i].z += METER_PER_PIXEL * clients[i].look_vector.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vector.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}

					}
				}
				if (clients[i].move_right) {
					if (clients[i].is_running && clients[i].stamina > 0.f) {
						//clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vector.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						//clients[i].x += METER_PER_PIXEL * clients[i].look_vector.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].stamina -= 0.1f;

						if (31.f <= clients[i].x &&clients[i].x < 34.f && -92.f <= clients[i].z && clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && 26.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -153.f && -92.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -92.f <= clients[i].z && clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= clients[i].x && clients[i].x <= 33.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -180.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -154.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (clients[i].x > 255.f) {
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							//clients[i].z += -METER_PER_PIXEL * clients[i].look_vector.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
							//clients[i].x += METER_PER_PIXEL * clients[i].look_vector.x * (RUN_SPEED * overlapped_buffer->elapsed_time);

							clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vector.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * clients[i].look_vector.z * (RUN_SPEED * overlapped_buffer->elapsed_time);

						}

					}
					else {
						//clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vector.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						//clients[i].x += METER_PER_PIXEL * clients[i].look_vector.z * (WALK_SPEED * overlapped_buffer->elapsed_time);

						if (31.f <= clients[i].x &&clients[i].x < 34.f && -92.f <= clients[i].z && clients[i].z <= 28) {
							//printf("1번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && 26.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("2번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -153.f && -92.f <= clients[i].z && clients[i].z <= 28.f) {
							//printf("3번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -92.f <= clients[i].z && clients[i].z <= -89.f) {
							//printf("4번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f,0.f, -1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (30.f <= clients[i].x && clients[i].x <= 33.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("5번 벽 부닺침\n");
							glm::vec3 v3Normal = { 1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= 33.f && -180.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("6번 벽 부닺침\n");
							glm::vec3 v3Normal = { 0.f, 0.f, 1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-156.f <= clients[i].x && clients[i].x <= -154.f && -256.f <= clients[i].z && clients[i].z <= -178.f) {
							//printf("7번 벽 부닺침\n");
							glm::vec3 v3Normal = { -1.f, 0.f, 0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (clients[i].x > 255.f) {
							printf("빠꾸치다가 넘었다\n");
							glm::vec3 v3Normal = { -1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].x) {
							glm::vec3 v3Normal = { 1.f,0.f,0.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (-255.f > clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else if (255.f < clients[i].z) {
							glm::vec3 v3Normal = { 0.f,0.f,-1.f };
							glm::vec3 v3Sliding = clients[i].look_vector - v3Normal * (glm::dot(clients[i].look_vector, v3Normal));
							clients[i].z += -METER_PER_PIXEL * v3Sliding.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * v3Sliding.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						}
						else {
							//clients[i].z += -METER_PER_PIXEL * clients[i].look_vector.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
							//clients[i].x += METER_PER_PIXEL * clients[i].look_vector.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vector.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
							clients[i].x += METER_PER_PIXEL * clients[i].look_vector.z * (WALK_SPEED * overlapped_buffer->elapsed_time);

						}

					}
				}

				clients[i].lock_server.unlock();
			}

			if (clients[client_id].is_use) {
				SC_PACKET_POS packets;
				packets.id = client_id;
				packets.size = sizeof(SC_PACKET_POS);
				packets.type = SC_POS;
				clients[client_id].y = height_map->GetHeight(clients[client_id].x + DX12_TO_OPGL, clients[client_id].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
				packets.x = clients[client_id].x;
				packets.y = clients[client_id].y;
				packets.z = clients[client_id].z;
				packets.stamina = clients[client_id].stamina;
				for (int i = 0; i < MAX_PLAYER; ++i) {
					if (clients[i].is_use == true) {
						SendPacket(i, &packets);
					}
				}
				ZeroMemory(overlapped_buffer, sizeof(OverlappedExtensionSet));
			}
		}
		else if (overlapped_buffer->evt_type == EVT_COLLISION) {
			// 보트아이템과 플레이어
			//lock_boat_item.lock();
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (clients[i].is_use) {
					for (int j = 0; j < 4; ++j) {
						if (item_boat[j].is_use && clients[i].hp > 0.f) {
							float fDist =
								(item_boat[j].x - clients[i].x)*(item_boat[j].x - clients[i].x) +
								(item_boat[j].y - clients[i].y)*(item_boat[j].y - clients[i].y) +
								(item_boat[j].z - clients[i].z)*(item_boat[j].z - clients[i].z);
							float fDistOrigin = (RAD_PLAYER + RAD_ITEM) * (RAD_PLAYER + RAD_ITEM);
							// 충돌
							if (fDist < fDistOrigin) {
								SC_PACKET_GET_ITEM packets;
								packets.size = sizeof(SC_PACKET_GET_ITEM);
								packets.type = SC_PLAYER_GET_ITEM;
								packets.m_cItemType = j;
								packets.m_cGetterID = i;
								clients[i].gathered_parts[j] = true;
								item_boat[j].is_use = false;
								//printf("%d플레이어와 %d아이템 충돌\n", i, j);
								for (int k = 0; k < MAX_PLAYER; ++k) {
									SendPacket(k, &packets);
								}
							}
						}
					}
				}
			}
			//lock_boat_item.unlock();

			// Ammo 아이템과 플레이어
			for (int i = 0; i < MAX_PLAYER; ++i) {
				for (int j = 0; j < 8; ++j) {
					if (clients[i].is_use && item_ammo[j].is_use) {
						float fDist =
							(item_ammo[j].x - clients[i].x)*(item_ammo[j].x - clients[i].x) +
							(item_ammo[j].y - clients[i].y)*(item_ammo[j].y - clients[i].y) +
							(item_ammo[j].z - clients[i].z)*(item_ammo[j].z - clients[i].z);
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
							//clients[i].gathered_parts[j] = true;
							clients[i].ammo_total = 90;
							item_ammo[j].is_use = false;
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
				lock_bullet[i].lock();
				for (int j = 1; j <= MAX_AMMO; ++j) {
					for (int k = 0; k < MAX_PLAYER; ++k) {
						// i 플레이어의 총알과 다른 플레이어들을 서로 충돌체크 
						//
						//	i							k
						// 총알 쏘는 플레이어와 총알 맞는 플레이어 모두 존재하고 
						// 플레이어가 발사한 총알까지 존재 해야 이 함수 실행
						if (i == k)continue;
						if (game_mode == 0) {
							if (clients[i].is_use && clients[k].is_use && bullets[i][j].is_use &&
								clients[i].team != clients[k].team) {
								float fDist =
									(bullets[i][j].x - clients[k].x)*(bullets[i][j].x - clients[k].x) +
									(bullets[i][j].y - clients[k].y)*(bullets[i][j].y - clients[k].y) +
									(bullets[i][j].z - clients[k].z)*(bullets[i][j].z - clients[k].z);
								float fDistOrigin = (RAD_PLAYER + RAD_BULLET) * (RAD_PLAYER + RAD_BULLET);
								// 충돌
								if (fDist < fDistOrigin) {
									SC_PACKET_HIT packets;
									packets.size = sizeof(SC_PACKET_HIT);
									packets.type = SC_HIT;
									clients[k].hp -= 10.f;
									packets.hp = clients[k].hp;
									packets.m_cBulletNumber = j;
									packets.m_cShooterID = i;
									packets.m_cHitID = k;
									bullets[i][j].is_use = false;
									//printf("%d플레이어와 %d플레이어의 총알 충돌\n", k, i);
									//printf("후 HP : %f\n", packets.hp);
									for (int l = 0; l < MAX_PLAYER; ++l) {
										SendPacket(l, &packets);
									}
								}
							}
						}
						else {
							if (clients[i].is_use && clients[k].is_use && bullets[i][j].is_use) {
								float fDist =
									(bullets[i][j].x - clients[k].x)*(bullets[i][j].x - clients[k].x) +
									(bullets[i][j].y - clients[k].y)*(bullets[i][j].y - clients[k].y) +
									(bullets[i][j].z - clients[k].z)*(bullets[i][j].z - clients[k].z);
								float fDistOrigin = (RAD_PLAYER + RAD_BULLET) * (RAD_PLAYER + RAD_BULLET);
								// 충돌
								if (fDist < fDistOrigin) {
									SC_PACKET_HIT packets;
									packets.size = sizeof(SC_PACKET_HIT);
									packets.type = SC_HIT;
									clients[k].hp -= 10.f;
									packets.hp = clients[k].hp;
									packets.m_cBulletNumber = j;
									packets.m_cShooterID = i;
									packets.m_cHitID = k;
									bullets[i][j].is_use = false;
									//printf("%d플레이어와 %d플레이어의 총알 충돌\n", k, i);
									//printf("후 HP : %f\n", packets.hp);
									for (int l = 0; l < MAX_PLAYER; ++l) {
										SendPacket(l, &packets);
									}
								}
							}
						}
					}
				}
				lock_bullet[i].unlock();
			}



			// 지면과 충돌
			for (int i = 0; i < MAX_PLAYER; ++i) {
				lock_bullet[i].lock();
				for (int j = 1; j <= MAX_AMMO; ++j) {
					if (bullets[i][j].is_use) {
						if (bullets[i][j].y < height_map->GetHeight(bullets[i][j].x + DX12_TO_OPGL, bullets[i][j].z + DX12_TO_OPGL)) {
							if (clients[i].is_use) {
								SC_PACKET_COLLISION_TB packets;
								packets.size = sizeof(SC_PACKET_COLLISION_TB);
								packets.type = SC_COLLISION_TB;
								packets.m_cBulletID = j;
								packets.m_cPlayerID = i;
								bullets[i][j].is_use = false;
								SendPacket(i, &packets);
								continue;
							}
						}
					}
				}
				lock_bullet[i].unlock();
			}
		}
		else if (overlapped_buffer->evt_type == EVT_BULLET_GENERATE) {
			int shooter_id = overlapped_buffer->shooter_player_id;
			if (clients[shooter_id].weapon == 0 && clients[shooter_id].is_use && clients[shooter_id].hp > 0.f) {
				lock_bullet[shooter_id].lock();
				if (clients[shooter_id].ammo_current == 1) {
					for (int d = 1; d <= MAX_AMMO; ++d) {
						bullets[shooter_id][d].is_use = false;
					}
				}
				bullets[shooter_id][clients[shooter_id].ammo_current].x = clients[shooter_id].x;
				bullets[shooter_id][clients[shooter_id].ammo_current].y = clients[shooter_id].y - 5.f;
				bullets[shooter_id][clients[shooter_id].ammo_current].z = clients[shooter_id].z;
				bullets[shooter_id][clients[shooter_id].ammo_current].look_vector.x = clients[shooter_id].look_vector.x;
				bullets[shooter_id][clients[shooter_id].ammo_current].look_vector.y = clients[shooter_id].look_vector.y;
				bullets[shooter_id][clients[shooter_id].ammo_current].look_vector.z = clients[shooter_id].look_vector.z;
				bullets[shooter_id][clients[shooter_id].ammo_current].time_ballistics = 0.f;
				bullets[shooter_id][clients[shooter_id].ammo_current].is_use = true;
				lock_bullet[shooter_id].unlock();
				//printf("%d가 발사한 총알 : %d [X : %f, Y : %f, Z : %f, InUse : %d ]\n", shooter_id, clients[shooter_id].ammo_current,
				//	bullets[shooter_id][clients[shooter_id].ammo_current].x,
				//	bullets[shooter_id][clients[shooter_id].ammo_current].y,
				//	bullets[shooter_id][clients[shooter_id].ammo_current].z,
				//	bullets[shooter_id][clients[shooter_id].ammo_current].is_use);
				//printf("실제 클라이언트 위치 [%f, %f, %f] \n", clients[shooter_id].x, clients[shooter_id].y, clients[shooter_id].z);
				clients[shooter_id].ammo_current--;
				// 남은 탄창 보내주기 
				SC_PACKET_AMMO packets;
				packets.size = sizeof(SC_PACKET_AMMO);
				packets.type = SC_AMMO;
				lock_ammo[shooter_id].lock();
				packets.ammo_current = clients[shooter_id].ammo_current;
				packets.ammo_total = clients[shooter_id].ammo_total;
				lock_ammo[shooter_id].unlock();
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
				if (clients[i].is_use)
					SendPacket(i, &packets);
			}
		}
		else if (overlapped_buffer->evt_type == EVT_BULLET_UPDATE) {
			// i 가 플레이어
			// j 가 플레이어가 발사한 총알
			for (int i = 0; i < MAX_PLAYER; ++i) {
				for (int j = 1; j <= MAX_AMMO; ++j) {
					if (clients[i].is_use) {
						lock_bullet[i].lock();
						if (bullets[i][j].is_use) {
							bullets[i][j].time_ballistics += overlapped_buffer->elapsed_time;
							bullets[i][j].x += (-1) * METER_PER_PIXEL * bullets[i][j].look_vector.x * (AR_SPEED * overlapped_buffer->elapsed_time);
							bullets[i][j].y += (-1) * METER_PER_PIXEL * bullets[i][j].look_vector.y * (AR_SPEED * overlapped_buffer->elapsed_time);
							bullets[i][j].y -= (9.8 / 2.f) * bullets[i][j].time_ballistics * bullets[i][j].time_ballistics / (AR_SPEED * 2);
							bullets[i][j].z += (-1) * METER_PER_PIXEL * bullets[i][j].look_vector.z * (AR_SPEED * overlapped_buffer->elapsed_time);

							if (bullets[i][j].y <= 0.f) {
								bullets[i][j].is_use = false;
							}
							else if (bullets[i][j].x >= 265.f || bullets[i][j].x <= -265.f) {
								bullets[i][j].is_use = false;
							}
							else if (bullets[i][j].z >= 265.f || bullets[i][j].z <= -265.f) {
								bullets[i][j].is_use = false;
							}
							
							SC_PACKET_BULLET packets;
							packets.id = i;
							packets.size = sizeof(SC_PACKET_BULLET);
							packets.type = SC_BULLET_POS;
							packets.bullet_id = j;
							packets.m_bInUse = bullets[i][j].is_use;
							packets.x = bullets[i][j].x;
							packets.y = bullets[i][j].y;
							packets.z = bullets[i][j].z;
							// 해당 플레이어에게만 보내야함
							for (int k = 0; k < MAX_PLAYER; ++k) {
								if (clients[k].is_use)
									SendPacket(k, &packets);
							}

						}
						lock_bullet[i].unlock();
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
	int retval = WSASend(clients[cl_id].s, &overlapped->wsabuf, 1, NULL, 0,
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
	closesocket(clients[cl_id].s);
	clients[cl_id].is_use = false;
	printf("[DisconnectPlayer] ClientID : %d\n", cl_id);
	SC_PACKET_REMOVE_PLAYER packet;
	packet.client_id = cl_id;
	packet.size = sizeof(SC_PACKET_REMOVE_PLAYER);
	packet.type = SC_REMOVE_PLAYER;

	// 플레이어가 나갔다는 정보를 모든 클라이언트에 뿌려준다.
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (clients[i].is_use == true) {
			SendPacket(i, &packet);
		}
	}

}

void ServerFramework::Update(duration<float>& elapsed_time) {

	//ol_ex[4].evt_type = EVT_PLAYER_POS_UPDATE;
	//ol_ex[4].elapsed_time = elapsed_time.count();
	//PostQueuedCompletionStatus(iocp_handle, 0, 4, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[4]));
	if (game_start) {
		ol_ex[5].evt_type = EVT_COLLISION;
		PostQueuedCompletionStatus(iocp_handle, 0, 5, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[5]));

		ol_ex[7].evt_type = EVT_BULLET_UPDATE;
		ol_ex[7].elapsed_time = elapsed_time.count();
		PostQueuedCompletionStatus(iocp_handle, 0, 7, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[7]));

		time_send += elapsed_time.count();
		if (time_send >= TIME_SEND_TIME) {
			ol_ex[9].evt_type = EVT_SEND_TIME;
			ol_ex[9].world_time = ITEM_time_boat - time_boat;
			PostQueuedCompletionStatus(iocp_handle, 0, 9, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[9]));
			time_send = 0.f;
		}

		if (is_boat_item_gen) {
			time_boat += elapsed_time.count();
			if (time_boat >= ITEM_time_boat) {
				ol_ex[8].evt_type = EVT_is_boat_item_gen;
				PostQueuedCompletionStatus(iocp_handle, 0, 0, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[8]));
				time_boat = 0.f;
				//is_boat_item_gen = false;
			}
		}

		if (ammo_gen) {
			time_ammo += elapsed_time.count();
			if (time_ammo >= ITEM_AMMO_GEN_TIME) {
				ol_ex[10].evt_type = EVT_AMMO_ITEM_GEN;
				PostQueuedCompletionStatus(iocp_handle, 0, 0, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[10]));
				time_ammo = 0.f;
				//ammo_gen = false;
			}
		}

		// 1초마다 플레이어 체력 갱신
		time_player_hp_update += elapsed_time.count();
		if (time_player_hp_update >= PLAYER_HP_UPDATE_TIME) {
			ol_ex[11].evt_type = EVT_PLAYER_HP_UPDATE;
			PostQueuedCompletionStatus(iocp_handle, 0, 11, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[11]));
			time_player_hp_update = 0.f;
		}


		sender_time += elapsed_time.count();
		if (sender_time >= UPDATE_TIME) {   // 1/60 초마다 데이터 송신
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (clients[i].move_backward || clients[i].move_foward || clients[i].move_left || clients[i].move_right) {
					ol_ex[i].evt_type = EVT_PLAYER_POS_SEND;
					ol_ex[i].elapsed_time = elapsed_time.count() + sender_time;
					PostQueuedCompletionStatus(iocp_handle, 0, i, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[i]));
				}
			}
			sender_time = 0.f;
		}


	}




}