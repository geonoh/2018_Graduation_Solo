#include "stdafx.h"
#include "ServerFramework.h"
#include "CHeightMapImage.h"
#include "Building.h"
#include "Object.h"
#include "Item.h"

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
	for (int i = 0; i < OBJECT_BUILDING; ++i) {
		delete building[i];
	}
	for (int i = 0; i < MAX_ITEM; ++i) {
		delete items[i];
	}
	delete height_map;
}

void ServerFramework::InitServer() {
#ifdef _Dev
	printf("---------------------------------\n");
	printf("- 개발모드\n");
	printf("---------------------------------\n");
	is_game_start = true;
#endif
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

	client_lock.lock();
	for (int i = 0; i < MAX_PLAYER; ++i) {
		clients[i].x = 0.f;
		clients[i].z = 0.f;
		clients[i].y = height_map->GetHeight(clients[i].x + DX12_TO_OPGL, clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
		clients[i].hp = 100.f;
		for (int j = 0; j < 4; ++j) {
			clients[i].boat_parts[j] = false;
		}
	}
	client_lock.unlock();

	for (int i = 0; i < MAX_PLAYER; ++i) {
		clients[i].SetOOBB(XMFLOAT3(clients[i].x, clients[i].y, clients[i].z), XMFLOAT3(OBB_SCALE_PLAYER_X, OBB_SCALE_PLAYER_Y, OBB_SCALE_PLAYER_Z), XMFLOAT4(0, 0, 0, 1));
		clients[i].bounding_box.Center;
	}

	for (int j = 0; j < MAX_PLAYER; ++j) {
		for (int i = 0; i < MAX_AMMO; ++i) {
			bullets[j][i].SetOOBB(XMFLOAT3(bullets[j][i].x, bullets[j][i].y, bullets[j][i].z),
				XMFLOAT3(OBB_SCALE_BULLET_X, OBB_SCALE_BULLET_Y, OBB_SCALE_BULLET_Z),
				XMFLOAT4(0, 0, 0, 1)); 
		}
	}

	XMFLOAT3 input_buffer[10];
	XMFLOAT3 input_extents[10];

	input_buffer[0] = XMFLOAT3{ 594.f,height_map->GetHeight(594.f,556.f) ,556.f };
	input_buffer[1] = XMFLOAT3{ 922.f,height_map->GetHeight(922.f,519.f) ,519.f };
	input_buffer[2] = XMFLOAT3{ 1152.f,height_map->GetHeight(1152.f,911.f) ,911.f };
	input_buffer[3] = XMFLOAT3{ 2168.f,height_map->GetHeight(2168.f,741.f) ,741.f };
	input_buffer[4] = XMFLOAT3{ 594.f,height_map->GetHeight(594.f,556.f) ,556.f };
	input_buffer[5] = XMFLOAT3{ 739.f,height_map->GetHeight(739.f,3526.f) ,3526.f };

	input_buffer[6] = XMFLOAT3{ 2516.f,height_map->GetHeight(2516.f,1589.f) ,1589.f };
	input_buffer[7] = XMFLOAT3{ 3071.f,height_map->GetHeight(3071.f,1906.f) ,1906.f };
	input_buffer[8] = XMFLOAT3{ 3251.f,height_map->GetHeight(3251.f,2721.f) ,2721.f };
	input_buffer[9] = XMFLOAT3{ 2106.f,height_map->GetHeight(2106.f,3322.f) ,3322.f };

	input_extents[0] = XMFLOAT3{ 100,100,100 };
	input_extents[1] = XMFLOAT3{ 100,100,100 };
	input_extents[2] = XMFLOAT3{ 100,100,100 };
	input_extents[3] = XMFLOAT3{ 100,100,100 };
	input_extents[4] = XMFLOAT3{ 100,100,100 };
	input_extents[5] = XMFLOAT3{ 100,100,100 };
	input_extents[6] = XMFLOAT3{ 100,100,100 };
	input_extents[7] = XMFLOAT3{ 100,100,100 };
	input_extents[8] = XMFLOAT3{ 100,100,100 };
	input_extents[9] = XMFLOAT3{ 100,100,100 };


	for (int i = 0; i < OBJECT_BUILDING; ++i) {
		input_extents[0] = XMFLOAT3{ 100,100,100 };
		building[i] = new Building;
		building[i]->SetOBB(input_buffer[i], input_extents[i]);
	}

	// 이건 됨
	XMFLOAT3 items_extents = XMFLOAT3{ 10.f,10.f,10.f };
	XMFLOAT3 init_item_pos = XMFLOAT3{ 0.f,0.f,0.f };
	for (int i = 0; i < MAX_ITEM; ++i) {
		items[i] = new Item;
		items[i]->in_use = false;
		items[i]->SetOBB(init_item_pos, items_extents);
	}

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
		if (clients[i].in_use == false) {
			client_id = i;
			break;
		}
	}
	if (client_id == -1) {
		printf("최대 유저 초과\n");
	}
	printf("[%d] 플레이어 입장\n", client_id);
	clients[client_id].s = client_socket;
	clients[client_id].ar_ammo = 30;
	clients[client_id].sub_ammo = 30;
	clients[client_id].ar_weapons = ARWeapons::NON_AR;
	clients[client_id].sub_weapons = SubWeapons::NON_SUB;
	clients[client_id].is_ready = false;
	clients[client_id].is_running = false;
	ZeroMemory(&clients[client_id].overlapped_ex.wsa_over, sizeof(WSAOVERLAPPED));
	//clients[client_id].overlapped_ex.is_recv = true;
	clients[client_id].overlapped_ex.evt_type = EVT_RECV_PACKET;
	clients[client_id].overlapped_ex.wsabuf.buf = clients[client_id].overlapped_ex.io_buffer;
	clients[client_id].overlapped_ex.wsabuf.len = sizeof(clients[client_id].overlapped_ex.io_buffer);
	clients[client_id].packet_size = 0;
	clients[client_id].prev_packet_size = 0;
	clients[client_id].team = Team::NON_TEAM;
	// 플레이어 입장 표시
	player_entered[client_id] = true;
	clients[client_id].in_use = true;

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
	packet.x = clients[client_id].x;
	packet.y = clients[client_id].y;
	packet.z = clients[client_id].z;
	SendPacket(client_id, &packet);

	// 나 제외 플레이어에게 입장정보 전송
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (clients[i].in_use && (client_id != i)) {
			printf("%d에게 %d의 정보를 보낸다\n", i, client_id);
			SendPacket(i, &packet);
		}
	}

	// 건물 정보 보내주기
	for (int j = 0; j < OBJECT_BUILDING; ++j) {
		SC_PACKET_ENTER_PLAYER packet_b;
		packet_b.id = j;
		packet_b.size = sizeof(SC_PACKET_ENTER_PLAYER);
		packet_b.type = SC_BUILDING_GEN;
		packet_b.x = building[j]->GetPosition().x;
		packet_b.y = building[j]->GetPosition().y;
		packet_b.z = building[j]->GetPosition().z;
		packet_b.size_x = building[j]->GetExtents().x;
		packet_b.size_y = building[j]->GetExtents().y;
		packet_b.size_z = building[j]->GetExtents().z;
		SendPacket(client_id, &packet_b);
	}

	// 다른 클라이언트의 위치 보내주기
	for (int i = 0; i < MAX_PLAYER; ++i) {
		ZeroMemory(&packet, sizeof(packet));
		if (i != client_id) {
			if (clients[i].in_use == true) {
				packet.id = i;
				packet.size = sizeof(SC_PACKET_ENTER_PLAYER);
				packet.type = SC_ENTER_PLAYER;
				clients[i].y = height_map->GetHeight(clients[i].x + DX12_TO_OPGL, clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
				packet.x = clients[i].x;
				packet.y = clients[i].y;
				packet.z = clients[i].z;
				SendPacket(client_id, &packet);
				printf("%d에게 %d의 정보를 보낸다\n", client_id, i);
			}
		}
	}

}

void ServerFramework::ProcessPacket(int cl_id, char* packet) {
	CS_PACKET_KEYUP* packet_buffer = reinterpret_cast<CS_PACKET_KEYUP*>(packet);

	switch (packet_buffer->type) {
	case CS_KEY_PRESS_UP:
		clients[cl_id].is_move_foward = true;
		break;
	case CS_KEY_PRESS_DOWN:
		printf("%d 클라에서 아래 가는거 눌렸음\n", cl_id);
		clients[cl_id].is_move_backward = true;
		break;
	case CS_KEY_PRESS_LEFT:
		clients[cl_id].is_move_left = true;
		break;
	case CS_KEY_PRESS_RIGHT:
		clients[cl_id].is_move_right = true;
		break;

	case CS_KEY_PRESS_1:
		printf("[ProcessPacket] :: AR 무기 선택\n");
		clients[cl_id].equipted_weapon = 0;
		break;
	case CS_KEY_PRESS_2:
		printf("[ProcessPacket] :: 권총 무기 선택\n");
		clients[cl_id].equipted_weapon = 1;
		break;

	case CS_KEY_PRESS_SHIFT:
		clients[cl_id].is_running = true;
		break;
	case CS_KEY_PRESS_SPACE:
		break;

	case CS_KEY_RELEASE_UP:
		clients[cl_id].is_move_foward = false;
		break;
	case CS_KEY_RELEASE_DOWN:
		clients[cl_id].is_move_backward = false;
		break;
	case CS_KEY_RELEASE_LEFT:
		clients[cl_id].is_move_left = false;
		break;
	case CS_KEY_RELEASE_RIGHT:
		clients[cl_id].is_move_right = false;
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
		clients[cl_id].is_right_click = true;
		break;
	case CS_RIGHT_BUTTON_UP:
		clients[cl_id].is_right_click = false;
		break;
	case CS_RELOAD:
		// AR 장착중인경우
		if (clients[cl_id].equipted_weapon == 0) {
			// 장전하려고 보니 결과가 0 이상
			if (clients[cl_id].ar_ammo - bullet_counter[cl_id] > 0) {
				clients[cl_id].ar_ammo -= bullet_counter[cl_id];
				bullet_counter[cl_id] = 0;
				printf("장전 완료, 남은 탄창 %d \n", clients[cl_id].ar_ammo);
				SC_PACKET_AMMO_O packets;
				packets.size = sizeof(SC_PACKET_AMMO_O);
				packets.type = SC_FULLY_AMMO;
				packets.ammo = bullet_counter[cl_id];
				SendPacket(cl_id, &packets);
			}
			// 장전하려고 보니 결과가 0 미만
			else {
				bullet_counter[cl_id] -= clients[cl_id].ar_ammo;
				clients[cl_id].ar_ammo = 0;
				printf("장전 완료, 남은 탄창 %d \n", clients[cl_id].ar_ammo);
				SC_PACKET_AMMO_O packets;
				packets.size = sizeof(SC_PACKET_AMMO_O);
				packets.type = SC_FULLY_AMMO;
				packets.ammo = bullet_counter[cl_id];
				SendPacket(cl_id, &packets);

			}
			//// 탄창에 30발 미만인경우 다 옮기고 ar_ammo를 0으로 만들어야 한다. 
			//if (clients[cl_id].ar_ammo <= MAX_AMMO) {
			//	//bullet_counter[cl_id] -= clients[cl_id].ar_ammo;
			//	clients[cl_id].ar_ammo -= bullet_counter[cl_id];
			//	clients[cl_id].ar_ammo = 0;
			//	printf("장전 완료, 남은 탄창 %d \n", clients[cl_id].ar_ammo);
			//	SC_PACKET_AMMO_O packets;
			//	packets.size = sizeof(SC_PACKET_AMMO_O);
			//	packets.type = SC_FULLY_AMMO;
			//	packets.ammo = bullet_counter[cl_id];
			//	SendPacket(cl_id, &packets);
			//}
			//// 탄창에 30발 이상 남은경우
			//else {
			//	clients[cl_id].ar_ammo -= bullet_counter[cl_id];
			//	bullet_counter[cl_id] = 0;
			//	printf("장전 완료, 남은 탄창 %d \n", clients[cl_id].ar_ammo);
			//	SC_PACKET_AMMO_O packets;
			//	packets.size = sizeof(SC_PACKET_AMMO_O);
			//	packets.type = SC_FULLY_AMMO;
			//	packets.ammo = bullet_counter[cl_id];
			//	SendPacket(cl_id, &packets);
			//}
		}
		// SUB 장착중인경우
		else {
			// 장전하려고 보니 결과가 0 이상
			if (clients[cl_id].sub_ammo - bullet_counter[cl_id] > 0) {
				clients[cl_id].sub_ammo -= bullet_counter[cl_id];
				bullet_counter[cl_id] = 0;
				printf("장전 완료, 남은 탄창 %d \n", clients[cl_id].sub_ammo);
				SC_PACKET_AMMO_O packets;
				packets.size = sizeof(SC_PACKET_AMMO_O);
				packets.type = SC_FULLY_AMMO;
				packets.ammo = bullet_counter[cl_id];
				SendPacket(cl_id, &packets);
			}
			// 장전하려고 보니 결과가 0 미만
			else {
				bullet_counter[cl_id] -= clients[cl_id].sub_ammo;
				clients[cl_id].sub_ammo = 0;
				printf("장전 완료, 남은 탄창 %d \n", clients[cl_id].sub_ammo);
				SC_PACKET_AMMO_O packets;
				packets.size = sizeof(SC_PACKET_AMMO_O);
				packets.type = SC_FULLY_AMMO;
				packets.ammo = bullet_counter[cl_id];
				SendPacket(cl_id, &packets);

			}

		}
		break;
	case CS_LEFT_BUTTON_DOWN:
		// 주무기
		if (clients[cl_id].equipted_weapon == 0) {
			if (bullet_counter[cl_id] == MAX_AMMO) {
				printf("총알 장전 필요\n");
				SC_PACKET_AMMO_O packets;
				packets.size = sizeof(SC_PACKET_AMMO_O);
				packets.type = SC_OUT_OF_AMMO;
				SendPacket(cl_id, &packets);
			}
			else {
				clients[cl_id].is_left_click = true;
				ol_ex[6].evt_type = EVT_BULLET_GENERATE;
				ol_ex[6].shooter_player_id = cl_id;
				//ol_ex[6].elapsed_time = elapsed_time.count();
				PostQueuedCompletionStatus(iocp_handle, 0, 6, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[6]));
			}
		}
		// 보조무기
		else if (clients[cl_id].equipted_weapon == 1) {
			if (bullet_counter[cl_id] == MAX_AMMO) {
				printf("총알 장전 필요\n");
				SC_PACKET_AMMO_O packets;
				packets.size = sizeof(SC_PACKET_AMMO_O);
				packets.type = SC_OUT_OF_AMMO;
				SendPacket(cl_id, &packets);
			}
			else {
				clients[cl_id].is_left_click = true;
				ol_ex[6].evt_type = EVT_BULLET_GENERATE;
				ol_ex[6].shooter_player_id = cl_id;
				//ol_ex[6].elapsed_time = elapsed_time.count();
				PostQueuedCompletionStatus(iocp_handle, 0, 6, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[6]));
			}
		}

		break;
	case CS_LEFT_BUTTON_UP:
		clients[cl_id].is_left_click = false;
		break;

	case CS_MOUSE_MOVE: {
		clients[cl_id].look_vec.x = packet_buffer->look_vec.x;
		clients[cl_id].look_vec.y = packet_buffer->look_vec.y;
		clients[cl_id].look_vec.z = packet_buffer->look_vec.z;
		SC_PACKET_LOOCVEC packets;
		packets.id = cl_id;
		packets.size = sizeof(SC_PACKET_LOOCVEC);
		packets.type = SC_PLAYER_LOOKVEC;
		packets.look_vec.x = clients[cl_id].look_vec.x;
		packets.look_vec.y = clients[cl_id].look_vec.y;
		packets.look_vec.z = clients[cl_id].look_vec.z;
		// 플레이어가 뒤는 상황

		//if (clients[cl_id].is_left_click) {
		//	packets.player_status = 3;
		//}
		//else if (clients[cl_id].is_running) {
		//	packets.player_status = 2;
		//}
		// 걷지도 뛰지도 않는 상황

		if (clients[cl_id].is_left_click) {
			packets.player_status = 2;
		}
		else if (clients[cl_id].is_move_backward == false && clients[cl_id].is_move_foward == false &&
			clients[cl_id].is_move_left == false && clients[cl_id].is_move_right == false) {
			packets.player_status = 0;
		}
		// 걷는 상황
		else if ((clients[cl_id].is_move_foward || clients[cl_id].is_move_left || clients[cl_id].is_move_right || clients[cl_id].is_move_backward)) {
			packets.player_status = 1;
		}
		
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].in_use == true) {
				SendPacket(i, &packets);
			}
		}
		break;
	}
	case CS_PLAYER_READY: {
		int ready_count = 0;
		printf("%d 플레이어 레디\n", cl_id);
		player_ready[cl_id] = true;
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (player_ready[i]) {
				ready_count++;
			}
		}
		if (ready_count == MAX_PLAYER) {
			GameStart();
		}
		break;
	}
	case CS_PLAYER_READY_CANCLE:
		printf("%d 플레이어 레디취소\n", cl_id);
		player_ready[cl_id] = false;
		break;
	case CS_PLAYER_TEAM_SELECT:
		break;
	}

}

void ServerFramework::GameStart() {
	printf("게임 시작\n");

	// 플레이어  위치 섞기
	for (int i = 0; i < MAX_PLAYER; ++i) {
		int dice = rand() % 4;
		switch (dice) {
		case MAP_AREA_1:
			printf("[%d] 플레이어 Area 1\n", i);
			clients[i].x = rand() % 1500;
			clients[i].z = rand() % 800 + 3000;
			break;
		case MAP_AREA_2:
			printf("[%d] 플레이어 Area 2\n", i);
			clients[i].x = rand() % 800 + 2000;
			clients[i].z = rand() % 1000 + 2200;
			break;
		case MAP_AREA_3:
			printf("[%d] 플레이어 Area 3\n", i);
			clients[i].x = rand() % 1500 + 1500;
			clients[i].z = rand() % 1500 + 400;
			break;
		case MAP_AREA_4:
			printf("[%d] 플레이어 Area 4\n", i);
			clients[i].x = rand() % 1500 + 100;
			clients[i].z = rand() % 1500 + 400;
			break;
		}
		clients[i].y = height_map->GetHeight(clients[i].x + DX12_TO_OPGL, clients[i].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
		clients[i].hp = 100.f;
	}

	// OOBB 셋
	for (int i = 0; i < MAX_PLAYER; ++i) {
		//clients[i].SetOOBB(XMFLOAT3(0, 0, 0), XMFLOAT3(10.f, 10.f, 10.f), XMFLOAT4(0, 0, 0, 1));
		clients[i].SetOOBB(XMFLOAT3(clients[i].x, clients[i].y, clients[i].z), XMFLOAT3(OBB_SCALE_PLAYER_X, OBB_SCALE_PLAYER_Y, OBB_SCALE_PLAYER_Z), XMFLOAT4(0, 0, 0, 1));
	}

	// Bullet의 OBB
	for (int j = 0; j < MAX_PLAYER; ++j) {
		for (int i = 0; i < MAX_AMMO; ++i) {
			bullets[j][i].SetOOBB(XMFLOAT3(bullets[j][i].x, bullets[j][i].y, bullets[j][i].z),
				XMFLOAT3(OBB_SCALE_BULLET_X, OBB_SCALE_BULLET_Y, OBB_SCALE_BULLET_Z),
				XMFLOAT4(0, 0, 0, 1));
		}
	}
	for (int i = 0; i < MAX_PLAYER; ++i) {
		ol_ex[i].evt_type = EVT_PLAYER_POS_SEND;
		PostQueuedCompletionStatus(iocp_handle, 0, i, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[i]));
	}

	//SC_PACKET_START packets;
	//packets.size = sizeof(SC_PACKET_START);
	//packets.type = SC_ITEM_GEN;
	//for (int i = 0; i < MAX_PLAYER; ++i) {
	//	if (clients[i].in_use == true) {
	//		SendPacket(i, &packets);
	//	}
	//}

	//ol_ex[9].evt_type = EVT_SEND_TIME;
	//ol_ex[9].world_time = world_time;
	//PostQueuedCompletionStatus(iocp_handle, 0, 9, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[9]));

	is_boat_item_gen = true;
	is_game_start = true;
	is_ammo_item_gen = true;

	time_game_start = GetTickCount();
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
		else if (overlapped_buffer->evt_type == EVT_AMMO_ITEM_GEN) {
			int ammo_item_gen = 0;
			for (int i = MAX_BOAT_ITEM; i < MAX_ITEM; ++i) {
				if (items[i]->in_use) {
					ammo_item_gen++;
				}
			}
			if (ammo_item_gen < MAX_ITEM - MAX_BOAT_ITEM) {
				SC_PACKET_ITEM_GEN packets;
				packets.size = sizeof(SC_PACKET_ITEM_GEN);
				packets.type = SC_ITEM_GEN;

				int dice;
				while (true) {
					dice = rand() % (MAX_ITEM - MAX_BOAT_ITEM) + MAX_BOAT_ITEM;
					if (items[dice]->in_use)
						continue;
					else
						break;
				}
				items[dice]->in_use = true;

				// dice가 짝수면 주무기
				// dice가 홀수면 보조무기
				// 참고로 Type은 0~3까지는 보트 부품
				// type 4 -> 주무기
				// type 5 -> 보조무기
				if (dice % 2 == 0) {
					items[dice]->SetItemType(4);
				}
				else {
					items[dice]->SetItemType(5);
				}
				packets.x = rand() % 4000;
				packets.z = rand() % 4000;
				packets.y = height_map->GetHeight(packets.x + DX12_TO_OPGL, packets.z + DX12_TO_OPGL) + PLAYER_HEIGHT;
				packets.item_type = dice;

				items[dice]->SetPosition(packets.x, packets.y, packets.z);

				// 부품의 타입도 정해야한다. 
				printf("[아이템 생성 : Type %d] : %f %f %f \n", items[dice]->GetItemType(), packets.x, packets.y, packets.z);
				for (int i = 0; i < MAX_PLAYER; ++i) {
					if (clients[i].in_use == true) {
						SendPacket(i, &packets);
					}
				}
				is_ammo_item_gen = true;
			}
		}
		else if (overlapped_buffer->evt_type == EVT_BOAT_ITEM_GEN) {
			int boat_item_gen = 0;
			for (int i = 0; i < MAX_BOAT_ITEM; ++i) {
				if (items[i]->in_use) {
					boat_item_gen++;
				}
			}
			if (boat_item_gen < MAX_BOAT_ITEM) {
				SC_PACKET_ITEM_GEN packets;
				packets.size = sizeof(SC_PACKET_ITEM_GEN);
				packets.type = SC_ITEM_GEN;

				int dice;
				while (true) {
					dice = rand() % MAX_BOAT_ITEM;
					if (items[dice]->in_use)
						continue;
					else
						break;
				}
				items[dice]->in_use = true;
				items[dice]->SetItemType(dice);
				packets.x = rand() % 4000;
				packets.z = rand() % 4000;
				packets.y = height_map->GetHeight(packets.x + DX12_TO_OPGL, packets.z + DX12_TO_OPGL) + PLAYER_HEIGHT;
				packets.item_type = dice;

				items[dice]->SetPosition(packets.x, packets.y, packets.z);

				// 부품의 타입도 정해야한다. 
				printf("[아이템 생성 : Type %d] : %f %f %f \n", items[dice]->GetItemType(), packets.x, packets.y, packets.z);
				for (int i = 0; i < MAX_PLAYER; ++i) {
					if (clients[i].in_use == true) {
						SendPacket(i, &packets);
					}
				}

				is_boat_item_gen = true;
			}

		}
		// TimerThread에서 호출
		// 1/20 마다 모든 플레이어에게 정보 전송
		else if (overlapped_buffer->evt_type == EVT_PLAYER_POS_SEND) {
			for (int i = 0; i < MAX_PLAYER; ++i) {
				//client_lock.lock();
				clients[i].client_lock.lock();
				if (clients[i].is_move_foward) {
					if (clients[i].is_running) {
						clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
					}
					else {
						clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
					}
				}
				if (clients[i].is_move_backward) {
					if (clients[i].is_running) {
						clients[i].z += METER_PER_PIXEL * clients[i].look_vec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += METER_PER_PIXEL * clients[i].look_vec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
					}
					else {
						//printf("%d플레이어 걷기 POS [%f, %f, %f] LV [%f, %f, %f]\n",
						//	i, clients[i].x, clients[i].y, clients[i].z,
						//	clients[i].look_vec.x, clients[i].look_vec.y, clients[i].look_vec.z);
						clients[i].z += METER_PER_PIXEL * clients[i].look_vec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += METER_PER_PIXEL * clients[i].look_vec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
					}

				}
				if (clients[i].is_move_left) {
					if (clients[i].is_running) {
						clients[i].z += METER_PER_PIXEL * clients[i].look_vec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
					}
					else {
						clients[i].z += METER_PER_PIXEL * clients[i].look_vec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
					}
				}
				if (clients[i].is_move_right) {
					if (clients[i].is_running) {
						clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += METER_PER_PIXEL * clients[i].look_vec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
					}
					else {
						clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += METER_PER_PIXEL * clients[i].look_vec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
					}
				}
				clients[i].client_lock.unlock();
				clients[i].SetOOBB(XMFLOAT3(clients[i].x, clients[i].y, clients[i].z), XMFLOAT3(OBB_SCALE_PLAYER_X, OBB_SCALE_PLAYER_Y, OBB_SCALE_PLAYER_Z), XMFLOAT4(0, 0, 0, 1));

			}

			if (clients[client_id].in_use) {
				SC_PACKET_POS packets;
				packets.id = client_id;
				packets.size = sizeof(SC_PACKET_POS);
				packets.type = SC_POS;
				clients[client_id].y = height_map->GetHeight(clients[client_id].x + DX12_TO_OPGL, clients[client_id].z + DX12_TO_OPGL) + PLAYER_HEIGHT;
				packets.x = clients[client_id].x;
				packets.y = clients[client_id].y;
				packets.z = clients[client_id].z;

				if (clients[client_id].is_left_click) {
					packets.player_status = 2;
				}
				else if (clients[client_id].is_move_backward == false && clients[client_id].is_move_foward == false &&
					clients[client_id].is_move_left == false && clients[client_id].is_move_right == false) {
					packets.player_status = 0;
				}
				// 걷는 상황
				else if ((clients[client_id].is_move_foward || clients[client_id].is_move_left || clients[client_id].is_move_right || clients[client_id].is_move_backward)) {
					packets.player_status = 1;
				}
				//packets.player_status = clients[client_id].is_running;
				//printf("높이 : %f\n", clients[client_id].y);
				for (int i = 0; i < MAX_PLAYER; ++i) {
					if (clients[i].in_use == true) {
						SendPacket(i, &packets);
					}
				}
				ZeroMemory(overlapped_buffer, sizeof(OverlappedExtensionSet));
			}
		}
		else if (overlapped_buffer->evt_type == EVT_COLLISION) {
			for (int i = 0; i < MAX_PLAYER; ++i) {
				for (int j = 0; j < MAX_BOAT_ITEM; ++j) {
					if (clients[i].in_use&&items[j]->in_use) {
						ContainmentType contain_type = clients[i].bounding_box.Contains(items[j]->bounding_box);
						//printf("items[%d]->bounding_box.center = [%lf, %lf, %lf] \n", j, items[j]->bounding_box.Center.x, items[j]->bounding_box.Center.y, items[j]->bounding_box.Center.z);
						switch (contain_type) {
						case INTERSECTS: {
							printf("얏 박앗당\n");
							clients[i].boat_parts[j] = true;
							int num_item_have = 0;
							for (int k = 0; k < 4; ++k) {
								if (clients[i].boat_parts[k]) {
									num_item_have++;
								}
							}
							if (num_item_have == 4) {
								printf("%d 플레이어의 승리\n", i);
							}
							break;
						}
						case CONTAINS:
							printf("얏 포함됫당\n");
							clients[i].boat_parts[j] = true;
							break;
						}
					}
				}
				for (int j = MAX_BOAT_ITEM; j < MAX_ITEM; ++j) {
					// 탄창 아이템 습득 
				}
			}

			// OBB 충돌체크 
			//for (int i = 0; i < OBJECT_BUILDING; ++i) {
			//	for (int j = 0; j < MAX_AMMO; ++j) {
			//		for (int k = 0; (k < MAX_PLAYER); ++k) {
			//			if (bullets[k][j].in_use) {
			//				ContainmentType contain_type = building[i]->bounding_box.Contains(bullets[k][j].bounding_box);
			//				switch (contain_type) {
			//				case DISJOINT:
			//					break;
			//				case INTERSECTS:
			//					SC_PACKET_COLLISION packets;
			//					packets.size = sizeof(SC_PACKET_COLLISION);
			//					packets.type = SC_COLLSION_BB;
			//					packets.x = clients[j].bounding_box.Center.x;
			//					// 플레이어의 키 만큼 반영해서
			//					packets.y = clients[j].bounding_box.Center.y;
			//					packets.z = clients[j].bounding_box.Center.z;
			//					packets.client_id = j;
			//					//
			//					// 플레이어 체력은 안깎아도 됭
			//					//clients[j].hp -= 25.f;
			//					//
			//					packets.hp = clients[j].hp;

			//					SendPacket(j, &packets);
			//					SendPacket(j + 1, &packets);
			//					bullets[k][j].in_use = false;

			//					printf("건물 총알 충돌 시작\n");
			//					break;
			//				case CONTAINS: {
			//					SC_PACKET_COLLISION packets;
			//					packets.size = sizeof(SC_PACKET_COLLISION);
			//					packets.type = SC_COLLSION_BB;
			//					packets.x = clients[j].bounding_box.Center.x;
			//					packets.y = clients[j].bounding_box.Center.y;
			//					packets.z = clients[j].bounding_box.Center.z;
			//					packets.client_id = j;

			//					// 플레이어 체력은 안깎아도 됨
			//					//clients[j].hp -= 25.f;
			//					packets.hp = clients[j].hp;

			//					SendPacket(j, &packets);
			//					SendPacket(j + 1, &packets);
			//					bullets[k][j].in_use = false;

			//					printf("건물 총알 충돌 !!!!!!!!!!!!!!\n");
			//					break;
			//				}
			//				}
			//			}
			//		}
			//	}
			//}
			for (int j = 0; (j < MAX_PLAYER - 1); ++j) {
				// 
				//for (int k = 0; k < OBJECT_BUILDING; ++k) {
				//	if (clients[j].in_use && clients[j + 1].in_use) {
				//		ContainmentType contain_type = building[k]->bounding_box.Contains(clients[j].bounding_box);
				//		switch (contain_type) {
				//		case DISJOINT:
				//			break;
				//		case INTERSECTS:
				//			SC_PACKET_COLLISION packets;
				//			packets.size = sizeof(SC_PACKET_COLLISION);
				//			packets.type = SC_COLLSION_BDP;
				//			packets.x = clients[j].bounding_box.Center.x;
				//			// 플레이어의 키 만큼 반영해서
				//			packets.y = clients[j].bounding_box.Center.y;
				//			packets.z = clients[j].bounding_box.Center.z;
				//			packets.client_id = j;
				//			//
				//			// 플레이어 체력은 안깎아도 됭
				//			//clients[j].hp -= 25.f;
				//			//
				//			packets.hp = clients[j].hp;

				//			SendPacket(j, &packets);
				//			SendPacket(j + 1, &packets);
				//			printf("건물과 충돌 시작\n");
				//			break;
				//		case CONTAINS: {
				//			SC_PACKET_COLLISION packets;
				//			packets.size = sizeof(SC_PACKET_COLLISION);
				//			packets.type = SC_COLLSION_BDP;
				//			packets.x = clients[j].bounding_box.Center.x;
				//			packets.y = clients[j].bounding_box.Center.y;
				//			packets.z = clients[j].bounding_box.Center.z;
				//			packets.client_id = j;

				//			// 플레이어 체력은 안깎아도 됨
				//			//clients[j].hp -= 25.f;
				//			packets.hp = clients[j].hp;

				//			SendPacket(j, &packets);
				//			SendPacket(j + 1, &packets);
				//			printf("건물과 충돌!!!!\n");
				//			break;
				//		}
				//		}
				//	}
				//}
				// 
				for (int i = 0; i < MAX_AMMO; ++i) {
					if (bullets[j + 1][i].in_use && clients[j].in_use) {
						ContainmentType containType = clients[j].bounding_box.Contains(bullets[j + 1][i].bounding_box);
						switch (containType)
						{
						case DISJOINT:
						{
							break;
						}
						case INTERSECTS:
						{
							SC_PACKET_COLLISION packets;
							packets.size = sizeof(SC_PACKET_COLLISION);
							packets.type = SC_COLLSION_PB;
							packets.x = clients[j].bounding_box.Center.x;
							// 플레이어의 키 만큼 반영해서
							packets.y = clients[j].bounding_box.Center.y;
							packets.z = clients[j].bounding_box.Center.z;
							packets.client_id = j;
							//
							clients[j].hp -= 25.f;
							//
							packets.hp = clients[j].hp;

							SendPacket(j, &packets);
							SendPacket(j + 1, &packets);
							printf("충돌 시작\n");
							bullets[j + 1][i].in_use = false;
							break;
						}
						case CONTAINS:
							SC_PACKET_COLLISION packets;
							packets.size = sizeof(SC_PACKET_COLLISION);
							packets.type = SC_COLLSION_PB;
							packets.x = clients[j].bounding_box.Center.x;
							packets.y = clients[j].bounding_box.Center.y;
							packets.z = clients[j].bounding_box.Center.z;
							packets.client_id = j;
							//
							clients[j].hp -= 25.f;
							//
							packets.hp = clients[j].hp;

							SendPacket(j, &packets);
							SendPacket(j + 1, &packets);
							printf("충돌!!!!\n");
							bullets[j + 1][i].in_use = false;
							break;
						}
					}
					if (bullets[j][i].in_use) {
						//ContainmentType containType_rev = clients[j].bounding_box.Contains(bullets[j + 1][i].bounding_box);
						ContainmentType containType_rev = bullets[j][i].bounding_box.Contains(clients[j + 1].bounding_box);
						switch (containType_rev)
						{
						case DISJOINT:
						{
							//printf("충돌 안함ㅠ\n");
							break;
						}
						case INTERSECTS:
						{
							SC_PACKET_COLLISION packets;
							packets.size = sizeof(SC_PACKET_COLLISION);
							packets.type = SC_COLLSION_PB;
							packets.x = clients[j + 1].bounding_box.Center.x;
							packets.y = clients[j + 1].bounding_box.Center.y;
							packets.z = clients[j + 1].bounding_box.Center.z;
							packets.client_id = j + 1;
							//
							clients[j + 1].hp -= 25.f;
							//
							packets.hp = clients[j + 1].hp;


							SendPacket(j, &packets);
							SendPacket(j + 1, &packets);
							bullets[j][i].in_use = false;
							printf("충돌 시작\n");
							break;
						}
						case CONTAINS:
							SC_PACKET_COLLISION packets;
							packets.size = sizeof(SC_PACKET_COLLISION);
							packets.type = SC_COLLSION_PB;
							packets.x = clients[j + 1].bounding_box.Center.x;
							packets.y = clients[j + 1].bounding_box.Center.y;
							packets.z = clients[j + 1].bounding_box.Center.z;
							packets.client_id = j + 1;
							//
							clients[j + 1].hp -= 25.f;
							//
							packets.hp = clients[j + 1].hp;

							SendPacket(j, &packets);
							SendPacket(j + 1, &packets);
							bullets[j][i].in_use = false;
							printf("충돌!!!!\n");
							break;
						}
					}
				}
			}
		}
		//}
		else if (overlapped_buffer->evt_type == EVT_BULLET_GENERATE) {
			int shooter_id = overlapped_buffer->shooter_player_id;
			if (clients[shooter_id].equipted_weapon == 0) {
				printf("%d가 발사한 총알 : %d\n", shooter_id, bullet_counter[shooter_id]);
				if (bullet_counter[shooter_id] == MAX_AMMO - 1) {
					for (int d = 0; d < MAX_AMMO; ++d) {
						bullets[shooter_id][d].in_use = false;
					}
				}
				bullets[shooter_id][bullet_counter[shooter_id]].x = clients[shooter_id].x;
				bullets[shooter_id][bullet_counter[shooter_id]].y = clients[shooter_id].y;
				bullets[shooter_id][bullet_counter[shooter_id]].z = clients[shooter_id].z;
				bullets[shooter_id][bullet_counter[shooter_id]].look_vec = clients[shooter_id].look_vec;
				bullets[shooter_id][bullet_counter[shooter_id]].in_use = true;
				bullet_counter[shooter_id]++;
				bullet_times[shooter_id] = 0;
			}
			else {

			}
		}
		else if (overlapped_buffer->evt_type == EVT_SEND_TIME) {
			SC_PACKET_TIME packets;
			packets.size = sizeof(SC_PACKET_TIME);
			packets.type = SC_WORLD_TIME;
			packets.world_time = overlapped_buffer->world_time;
			printf("시간 보냄 %f \n", packets.world_time);

			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (clients[i].in_use)
					SendPacket(i, &packets);
			}
		}
		else if (overlapped_buffer->evt_type == EVT_BULLET_UPDATE) {
			// i 가 플레이어
			// j 가 플레이어가 발사한 총알
			for (int i = 0; i < MAX_PLAYER; ++i) {
				for (int j = 0; j < MAX_AMMO; ++j) {
					if (bullets[i][j].in_use) {
						bullets[i][j].x += METER_PER_PIXEL * bullets[i][j].look_vec.x * (AR_SPEED * overlapped_buffer->elapsed_time);
						bullets[i][j].y += METER_PER_PIXEL * bullets[i][j].look_vec.y * (AR_SPEED * overlapped_buffer->elapsed_time);
						bullets[i][j].z += METER_PER_PIXEL * bullets[i][j].look_vec.z * (AR_SPEED * overlapped_buffer->elapsed_time);

						bullets[i][j].SetOOBB(
							XMFLOAT3(bullets[i][j].x, bullets[i][j].y, bullets[i][j].z),
							XMFLOAT3(OBB_SCALE_BULLET_X, OBB_SCALE_BULLET_Y, OBB_SCALE_BULLET_Z),
							XMFLOAT4(0, 0, 0, 1));
					}
					if (bullets[i][j].x >= 4000.f || bullets[i][j].x <= 0) {
						bullets[i][j].in_use = false;
						continue;
					}
					if (bullets[i][j].y >= 4000.f || bullets[i][j].y <= 0) {
						bullets[i][j].in_use = false;
						continue;
					}
					if (bullets[i][j].z >= 4000.f || bullets[i][j].z <= 0) {
						bullets[i][j].in_use = false;
						continue;
					}

					if (bullets[i][j].in_use) {
						SC_PACKET_BULLET packets;
						packets.id = i;
						packets.size = sizeof(SC_PACKET_BULLET);
						packets.type = SC_BULLET_POS;
						packets.bullet_id = j;
						packets.x = bullets[i][j].x;
						packets.y = bullets[i][j].y;
						packets.z = bullets[i][j].z;
						// 해당 플레이어에게만 보내야함
						SendPacket(i, &packets);
					}
				}
			}

		}
		// Send로 인해 할당된 영역 반납
		else if(overlapped_buffer->evt_type == EVT_SEND_PACKET){
			delete overlapped_buffer;
		}
	}
}

void ServerFramework::SendPacket(int cl_id, void* packet) {
	OverlappedExtensionSet* overlapped = new OverlappedExtensionSet;
	char* send_buffer = reinterpret_cast<char*>(packet);

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
	clients[cl_id].in_use = false;
	printf("[DisconnectPlayer] ClientID : %d\n", cl_id);
	SC_PACKET_REMOVE_PLAYER packet;
	packet.client_id = cl_id;
	packet.size = sizeof(SC_PACKET_REMOVE_PLAYER);
	packet.type = SC_REMOVE_PLAYER;

	// 플레이어가 나갔다는 정보를 모든 클라이언트에 뿌려준다.
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (clients[i].in_use == true) {
			SendPacket(i, &packet);
		}
	}

}

void ServerFramework::Update(duration<float>& elapsed_time) {

	//ol_ex[4].evt_type = EVT_PLAYER_POS_UPDATE;
	//ol_ex[4].elapsed_time = elapsed_time.count();
	//PostQueuedCompletionStatus(iocp_handle, 0, 4, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[4]));

	ol_ex[5].evt_type = EVT_COLLISION;
	PostQueuedCompletionStatus(iocp_handle, 0, 5, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[5]));

	ol_ex[7].evt_type = EVT_BULLET_UPDATE;
	ol_ex[7].elapsed_time = elapsed_time.count();
	PostQueuedCompletionStatus(iocp_handle, 0, 7, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[7]));




	sender_time += elapsed_time.count();
	if (sender_time >= UPDATE_TIME) {   // 1/60 초마다 데이터 송신
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (clients[i].is_move_backward || clients[i].is_move_foward || clients[i].is_move_left || clients[i].is_move_right) {
				ol_ex[i].evt_type = EVT_PLAYER_POS_SEND;
				ol_ex[i].elapsed_time = elapsed_time.count() + sender_time;
				PostQueuedCompletionStatus(iocp_handle, 0, i, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[i]));
			}
		}
		sender_time = 0.f;
	}

	if (is_game_start) {
		time_sender_time += elapsed_time.count();
		if (time_sender_time >= TIME_SEND_TIME) {
			ol_ex[9].evt_type = EVT_SEND_TIME;
			// chrono count의 경우 초 단위이므로 GetTickCount의 단위 (ms)를
			// 맞춰주기 위해서 1000을 곱해줘야한다. 
			if (ITEM_BOAT_GEN_TIME * 1000 - (GetTickCount() - time_game_start) < 0) {
				time_game_start = GetTickCount();
			}
			ol_ex[9].world_time = ITEM_BOAT_GEN_TIME * 1000 - (GetTickCount() - time_game_start);
			PostQueuedCompletionStatus(iocp_handle, 0, 9, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[9]));
			time_sender_time = 0.f;
		}
	}

	if (is_boat_item_gen) {
		item_boat_gen_timer += elapsed_time.count();
		if (item_boat_gen_timer >= ITEM_BOAT_GEN_TIME) {
			ol_ex[8].evt_type = EVT_BOAT_ITEM_GEN;
			PostQueuedCompletionStatus(iocp_handle, 0, 0, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[8]));
			item_boat_gen_timer = 0.f;
			is_boat_item_gen = false;
		}
	}

	if (is_ammo_item_gen) {
		item_ammo_gen_timer += elapsed_time.count();
		if (item_ammo_gen_timer >= ITEM_AMMO_GEN_TIME) {
			ol_ex[10].evt_type = EVT_AMMO_ITEM_GEN;
			PostQueuedCompletionStatus(iocp_handle, 0, 0, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[10]));
			item_ammo_gen_timer = 0.f;
			is_ammo_item_gen = false;
		}

	}

}