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
	printf("%s", msg);
	wprintf(L"에러%s\n", lpMsgBuf);
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
	int retval = 0;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		printf("WSAStartup() 에러\n");

	iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp_handle == NULL)
		printf("최초: CreateIoCompletionPort() 에러\n");

	// 비동기 방식의 Listen 소켓 생성
	listen_socket = WSASocketW(AF_INET, SOCK_STREAM,
		IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

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

	XMFLOAT3 xmf3Scale(8.0f, 1.f, 8.0f);
	LPCTSTR file_name = _T("terrain18.raw");
	height_map = new CHeightMapImage(file_name, 513, 513, xmf3Scale);

	client_lock.lock();
	for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
		clients[i].x = 450.f;
		clients[i].z = 800.f;
		clients[i].y = height_map->GetHeight(clients[i].x, clients[i].z) + PLAYER_HEIGHT;
		clients[i].hp = 100.f;
	}
	client_lock.unlock();

	// OOBB 셋
	for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
		//clients[i].SetOOBB(XMFLOAT3(0, 0, 0), XMFLOAT3(10.f, 10.f, 10.f), XMFLOAT4(0, 0, 0, 1));
		clients[i].SetOOBB(XMFLOAT3(clients[i].x, clients[i].y, clients[i].z), XMFLOAT3(OBB_SCALE_PLAYER_X, OBB_SCALE_PLAYER_Y, OBB_SCALE_PLAYER_Z), XMFLOAT4(0, 0, 0, 1));
	}

	// Bullet의 OBB
	for (int j = 0; j < MAXIMUM_PLAYER; ++j) {
		for (int i = 0; i < MAX_BULLET_SIZE; ++i) {
			bullets[j][i].SetOOBB(XMFLOAT3(bullets[j][i].x, bullets[j][i].y, bullets[j][i].z),
				XMFLOAT3(OBB_SCALE_BULLET_X, OBB_SCALE_BULLET_Y, OBB_SCALE_BULLET_Z),
				XMFLOAT4(0, 0, 0, 1));
		}
	}

	// 플레이어에게 위치 정보 보내는거 Timer화 시키기
	AddTimer(0, SS_PLAYER_MOVE, GetTickCount() + SEND_TERM);
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

	printf("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port));

	int client_id = -1;
	client_lock.lock();
	for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
		if (clients[i].in_use == false) {
			client_id = i;
			break;
		}
	}
	client_lock.unlock();
	if (client_id == -1) {
		printf("최대 유저 초과\n");
	}
	printf("[%d] 플레이어 입장\n", client_id);
	client_lock.lock();
	clients[client_id].s = client_socket;
	clients[client_id].ar_mag = 0;
	clients[client_id].sub_mag = 0;
	clients[client_id].ar_weapons = ARWeapons::NON_AR;
	clients[client_id].sub_weapons = SubWeapons::NON_SUB;
	client_lock.unlock();
	clients[client_id].is_ready = false;
	clients[client_id].is_running = false;
	ZeroMemory(&clients[client_id].overlapped_ex.wsa_over, sizeof(WSAOVERLAPPED));
	clients[client_id].overlapped_ex.is_recv = true;
	clients[client_id].overlapped_ex.wsabuf.buf = clients[client_id].overlapped_ex.io_buffer;
	clients[client_id].overlapped_ex.wsabuf.len = sizeof(clients[client_id].overlapped_ex.io_buffer);
	clients[client_id].packet_size = 0;
	clients[client_id].prev_packet_size = 0;
	clients[client_id].team = Team::NON_TEAM;

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(client_socket),
		iocp_handle, client_id, 0);
	// 플레이어 입장 표시
	player_entered[client_id] = true;
	clients[client_id].in_use = true;
	unsigned long flag = 0;
	WSARecv(client_socket, &clients[client_id].overlapped_ex.wsabuf, 1, NULL,
		&flag, &clients[client_id].overlapped_ex.wsa_over, NULL);

	// 플레이어 입장했다고 패킷 보내줘야함.
	// 이 정보에는 플레이어의 초기 위치정보도 포함되어야 한다. 
	SC_PACKET_ENTER_PLAYER packet;
	packet.id = client_id;
	packet.size = sizeof(SC_PACKET_ENTER_PLAYER);
	packet.type = SC_ENTER_PLAYER;
	packet.x = clients[client_id].x;
	packet.y = clients[client_id].y;
	packet.z = clients[client_id].z;
	SendPacket(client_id, &packet);
	for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
		if (clients[i].in_use && (client_id != i)) {
			printf("%d 플레이어 입장 정보 전송\n", i);
			SendPacket(i, &packet);
		}
	}

	// 해당 클라이언트에게도 다른 클라이언트의 위치를 보내줘야한당!~
	for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
		ZeroMemory(&packet, sizeof(packet));
		if (i != client_id) {
			if (clients[i].in_use == true) {
				packet.id = i;
				packet.size = sizeof(SC_PACKET_ENTER_PLAYER);
				packet.type = SC_ENTER_PLAYER;
				clients[i].y = height_map->GetHeight(clients[i].x, clients[i].z);
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
		clients[cl_id].look_vec = packet_buffer->look_vec;

		printf("%d 플레이어의 Look Vector %f, %f, %f\n",cl_id,clients[cl_id].look_vec.x,
			clients[cl_id].look_vec.y, clients[cl_id].look_vec.z);
		break;
	case CS_KEY_PRESS_DOWN:
		clients[cl_id].is_move_backward = true;
		clients[cl_id].look_vec = packet_buffer->look_vec;
		break;
	case CS_KEY_PRESS_LEFT:
		clients[cl_id].is_move_left = true;
		clients[cl_id].look_vec = packet_buffer->look_vec;
		break;
	case CS_KEY_PRESS_RIGHT:
		clients[cl_id].is_move_right = true;
		clients[cl_id].look_vec = packet_buffer->look_vec;
		break;

	case CS_KEY_PRESS_1:
		printf("[ProcessPacket] :: AR 무기 선택\n");
		break;
	case CS_KEY_PRESS_2:
		printf("[ProcessPacket] :: 권총 무기 선택\n");
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

	case CS_LEFT_BUTTON_DOWN:
		clients[cl_id].is_left_click = true;
		//bullet_counter[cl_id]++;
		break;
	case CS_LEFT_BUTTON_UP:
		clients[cl_id].is_left_click = false;
		break;

	case CS_MOUSE_MOVE: {
		clients[cl_id].look_vec = packet_buffer->look_vec;
		SC_PACKET_LOOCVEC packets;
		packets.id = cl_id;
		packets.size = sizeof(SC_PACKET_LOOCVEC);
		packets.type = SC_PLAYER_LOOKVEC;
		packets.look_vec = clients[cl_id].look_vec;
		for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
			if (clients[i].in_use == true) {
				SendPacket(i, &packets);
			}
		}
		break;
	}
	case CS_PLAYER_READY: {
		ol_ex[8].command = SS_PLAYER_READY;
		PostQueuedCompletionStatus(iocp_handle, 0, cl_id, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[8]));
		break;
	}
	case CS_PLAYER_READY_CANCLE:
		ol_ex[9].command = CS_PLAYER_READY_CANCLE;
		PostQueuedCompletionStatus(iocp_handle, 0, cl_id, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[9]));
		break;
	case CS_PLAYER_TEAM_SELECT:
		break;
	}

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
		if (overlapped_buffer->is_recv == true) {
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
					ErrorDisplay("Error in WorkerThread(Recv) : ", err_no);
				}
			}

		}
		else if (overlapped_buffer->command == SS_PLAYER_MOVE) {
			for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
				if (clients[i].in_use) {
					if (clients[i].is_move_backward || clients[i].is_move_foward || clients[i].is_move_left || clients[i].is_move_right) {
						SC_PACKET_POS packets;
						packets.id = i;
						packets.size = sizeof(SC_PACKET_POS);
						packets.type = SC_POS;
						clients[i].y = height_map->GetHeight(clients[i].x, clients[i].z) + PLAYER_HEIGHT;
						packets.x = clients[i].x;
						packets.y = clients[i].y;
						packets.z = clients[i].z;
						//printf("높이 : %f\n", clients[client_id].y);
						for (int j = 0; j < MAXIMUM_PLAYER; ++j) {
							if (clients[j].in_use == true) {
								SendPacket(j, &packets);
							}
						}

					}
				}
			}

			AddTimer(0, SS_PLAYER_MOVE, GetTickCount() + SEND_TERM);
			delete overlapped_buffer;
		}
		else if (overlapped_buffer->command == SS_PLAYER_READY) {
			int ready_count_buf = 0;
			player_ready[client_id] = true;
			printf("%d 플레이어 레디 했\n", client_id);

			// 전체 플레이어 들어왔는지 확인 
			for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
				if (player_ready[i])
					ready_count_buf++;
			}
			if (ready_count_buf == MAXIMUM_PLAYER) {
				printf("모든플레이어 레디 게임 시작\n");
				printf("2분 후 아이템 드랍이 시작 됩니다. \n");
				AddTimer(0, SS_ITEM_GEN, GetTickCount() + ITEM_GEN_TIME);
			}
		}
		else if (overlapped_buffer->command == SS_ITEM_GEN) {
			printf("아이템 짠짠★\n");

			// 첫 번째 부품 생성 후 그 이후 부품을 생성해야한다. 
			printf("2분 후 아이템 드랍이 시작 됩니다. \n");
			AddTimer(0, SS_ITEM_GEN, GetTickCount() + ITEM_GEN_TIME);
		}
		else if (overlapped_buffer->command == CS_PLAYER_READY_CANCLE) {
			player_ready[client_id] = true;
			printf("%d 플레이어 레디 풂\n", client_id);

			// 전체 플레이어 들어왔는지 확인 
			for (int i = 0; i < MAXIMUM_PLAYER; ++i) {

			}
		}
		// TimerThread에서 호출
		// 1/20 마다 모든 플레이어에게 정보 전송
		else if (overlapped_buffer->command == SC_PLAYER_MOVE) {
			if (clients[client_id].in_use) {
				SC_PACKET_POS packets;
				packets.id = client_id;
				packets.size = sizeof(SC_PACKET_POS);
				packets.type = SC_POS;
				clients[client_id].y = height_map->GetHeight(clients[client_id].x, clients[client_id].z) + PLAYER_HEIGHT;
				packets.x = clients[client_id].x;
				packets.y = clients[client_id].y;
				packets.z = clients[client_id].z;
				//printf("높이 : %f\n", clients[client_id].y);
				for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
					if (clients[i].in_use == true) {
						SendPacket(i, &packets);
					}
				}
				ZeroMemory(overlapped_buffer, sizeof(OverlappedExtensionSet));
			}
		}
		else if (overlapped_buffer->command == SS_COLLISION) {
			// OBB 충돌체크  
			for (int j = 0; j < MAXIMUM_PLAYER - 1; ++j) {
				for (int i = 0; i < MAX_BULLET_SIZE; ++i) {
					if (bullets[j + 1][i].in_use) {
						ContainmentType containType = clients[j].bounding_box.Contains(bullets[j + 1][i].bounding_box);
						switch (containType)
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
							packets.x = clients[j].bounding_box.Center.x;
							packets.y = clients[j].bounding_box.Center.y + PLAYER_HEIGHT;
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
		else if (overlapped_buffer->command == SS_PLAYER_POS_UPDATE) {
			for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
				//client_lock.lock();
				clients[i].client_lock.lock();
				if (clients[i].is_move_foward) {
					if (clients[i].is_running) {
						clients[i].z += METER_PER_PIXEL * clients[i].look_vec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += METER_PER_PIXEL * clients[i].look_vec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
					}
					else {
						clients[i].z += METER_PER_PIXEL * clients[i].look_vec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += METER_PER_PIXEL * clients[i].look_vec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
					}
				}
				if (clients[i].is_move_backward) {
					if (clients[i].is_running) {
						clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vec.z * (RUN_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vec.x * (RUN_SPEED * overlapped_buffer->elapsed_time);
					}
					else {
						clients[i].z += (-1) * METER_PER_PIXEL * clients[i].look_vec.z * (WALK_SPEED * overlapped_buffer->elapsed_time);
						clients[i].x += (-1) * METER_PER_PIXEL * clients[i].look_vec.x * (WALK_SPEED * overlapped_buffer->elapsed_time);
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
				//client_lock.unlock();

				XMFLOAT4X4 danwi(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, clients[i].x, height_map->GetHeight(clients[i].x, clients[i].z) + PLAYER_HEIGHT, clients[i].z, 1);
				clients[i].bounding_box.Transform(clients[i].bounding_box,
					DirectX::XMLoadFloat4x4(&danwi));
				XMStoreFloat4(&clients[i].bounding_box.Orientation, XMQuaternionNormalize(XMLoadFloat4(&clients[i].bounding_box.Orientation)));
				clients[i].bounding_box.Extents.x = OBB_SCALE_PLAYER_X;
				clients[i].bounding_box.Extents.y = OBB_SCALE_PLAYER_Y;
				clients[i].bounding_box.Extents.z = OBB_SCALE_PLAYER_Z;
			}
		}
		else if (overlapped_buffer->command == SS_BULLET_GENERATE) {
			for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
				if (clients[i].is_left_click) {
					bullet_lock.lock();
					bullet_times[i] += overlapped_buffer->elapsed_time;
					if (bullet_times[i] >= AR_SHOOTER) {
						if (bullet_counter[i] > MAX_BULLET_SIZE - 2) {
							for (int d = 0; d < MAX_BULLET_SIZE; ++d) {
								bullets[i][d].in_use = false;
							}
							bullet_counter[i] = 0;
							printf("총알 초기화\n");
							//break;
						}
						bullets[i][bullet_counter[i]].x = clients[i].x;
						bullets[i][bullet_counter[i]].y = clients[i].y;
						bullets[i][bullet_counter[i]].z = clients[i].z;
						bullets[i][bullet_counter[i]].look_vec = clients[i].look_vec;
						bullets[i][bullet_counter[i]].in_use = true;
						bullet_counter[i]++;
						bullet_times[i] = 0;
					}
					bullet_lock.unlock();
				}
			}
		}
		else if (overlapped_buffer->command == SS_BULLET_UPDATE) {
			// i 가 플레이어
			// j 가 플레이어가 발사한 총알
			for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
				for (int j = 0; j < MAX_BULLET_SIZE; ++j) {
					//bullet_lock.lock();
					if (bullets[i][j].in_use) {
						bullets[i][j].x += METER_PER_PIXEL * bullets[i][j].look_vec.x * (AR_SPEED * overlapped_buffer->elapsed_time);
						bullets[i][j].y += METER_PER_PIXEL * bullets[i][j].look_vec.y * (AR_SPEED * overlapped_buffer->elapsed_time);
						bullets[i][j].z += METER_PER_PIXEL * bullets[i][j].look_vec.z * (AR_SPEED * overlapped_buffer->elapsed_time);
						//printf("총알 진행중\n");

						XMFLOAT4X4 danwi(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, bullets[i][j].x, bullets[i][j].y, bullets[i][j].z, 1);
						bullets[i][j].bounding_box.Transform(bullets[i][j].bounding_box,
							DirectX::XMLoadFloat4x4(&danwi));
						XMStoreFloat4(&bullets[i][j].bounding_box.Orientation, XMQuaternionNormalize(XMLoadFloat4(&bullets[i][j].bounding_box.Orientation)));
						bullets[i][j].bounding_box.Extents.x = OBB_SCALE_BULLET_X;
						bullets[i][j].bounding_box.Extents.y = OBB_SCALE_BULLET_Y;
						bullets[i][j].bounding_box.Extents.z = OBB_SCALE_BULLET_Z;

					}
					if (bullets[i][j].x >= 4000.f || bullets[i][j].x <= 0) {
						bullets[i][j].in_use = false;
						//bullet_lock.unlock();
						continue;
					}
					if (bullets[i][j].y >= 4000.f || bullets[i][j].y <= 0) {
						bullets[i][j].in_use = false;
						//bullet_lock.unlock();
						continue;
					}
					if (bullets[i][j].z >= 4000.f || bullets[i][j].z <= 0) {
						bullets[i][j].in_use = false;
						//bullet_lock.unlock();
						continue;
					}

					//여기서 보내줘야지~
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
					//bullet_lock.unlock();
				}
			}

		}
		// Send로 인해 할당된 영역 반납
		else {
			delete overlapped_buffer;
		}
	}
}
void ServerFramework::SendPacket(int cl_id, void* packet) {
	OverlappedExtensionSet* overlapped = new OverlappedExtensionSet;
	char* send_buffer = reinterpret_cast<char*>(packet);

	memcpy(&overlapped->io_buffer, packet, send_buffer[0]);
	overlapped->is_recv = false;
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

	player_ready[cl_id] = false;


	clients[cl_id].in_use = false;
	printf("[DisconnectPlayer] ClientID : %d\n", cl_id);
	SC_PACKET_REMOVE_PLAYER packet;
	packet.client_id = cl_id;
	packet.size = sizeof(SC_PACKET_REMOVE_PLAYER);
	packet.type = SC_REMOVE_PLAYER;

	// 플레이어가 나갔다는 정보를 모든 클라이언트에 뿌려준다.
	for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
		if (clients[i].in_use == true) {
			SendPacket(i, &packet);
		}
	}

}

bool ServerFramework::IsStartGame() {
	int ready_counter = 0;
	for (auto i = 0; i < MAXIMUM_PLAYER; ++i) {
		if (player_ready[i] == true) {
			printf("%d번 플레이어 레디\n", i);
			ready_counter++;
		}
	}
	if (ready_counter == 4)
		return true;
	else
		return false;
}

void ServerFramework::Update(duration<float>& elapsed_time) {

	Sleep(1);   // 이거 붙여야 뒤쪽 이동할때 잘 가는데
				// 이유를 모르겠네 도대체;
	ol_ex[4].command = SS_PLAYER_POS_UPDATE;
	ol_ex[4].elapsed_time = elapsed_time.count();
	PostQueuedCompletionStatus(iocp_handle, 0, 4, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[4]));

	ol_ex[5].command = SS_COLLISION;
	PostQueuedCompletionStatus(iocp_handle, 0, 5, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[5]));

	// bool 변수 클라이언트마다 배정.
	// 시간값을 overlapped로 넘겨줘서 
	ol_ex[6].command = SS_BULLET_GENERATE;
	ol_ex[6].elapsed_time = elapsed_time.count();
	PostQueuedCompletionStatus(iocp_handle, 0, 6, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[6]));

	// Bullet이 실제로 날아가는건 여기서 관리해야할거같다.
	ol_ex[7].command = SS_BULLET_UPDATE;
	ol_ex[7].elapsed_time = elapsed_time.count();
	PostQueuedCompletionStatus(iocp_handle, 0, 7, reinterpret_cast<WSAOVERLAPPED*>(&ol_ex[7]));
}

void ServerFramework::TimerThread() {
	while (true) {
		Sleep(1);
		while (false == timer_queue.empty()) {
			if (timer_queue.top().start_time >= GetTickCount())
				break;
			Event ev = timer_queue.top();
			timer_queue.pop();
			OverlappedExtensionSet* ex = new OverlappedExtensionSet;
			ex->command = ev.type;
			PostQueuedCompletionStatus(iocp_handle, 1, ev.id, &ex->wsa_over);
		}
	}
}

void ServerFramework::AddTimer(int id, int type, unsigned int start_time) {
	timer_queue.push(Event{ id, type, start_time });
}