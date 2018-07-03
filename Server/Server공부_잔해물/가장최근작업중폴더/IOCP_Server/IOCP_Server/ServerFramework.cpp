#include "ServerFramework.h"


DWORD WINAPI ServerFramework::WorkerThread(LPVOID arg) {
	int retval = 0;
	HANDLE thread_hcp = (HANDLE)arg;

	while (true) {
		DWORD transferred_size;
		SOCKET client_socket_buffer;
		SocketInfo* socket_info_buffer;

		retval = GetQueuedCompletionStatus(thread_hcp, &transferred_size,		// FAIL뜨면 0을 return 함
			(PULONG_PTR)&client_socket_buffer, (LPOVERLAPPED*)&socket_info_buffer, INFINITE);

		SOCKADDR_IN clinet_address;
		int addrlen = sizeof(clinet_address);


		// -----------------------------------------------------------------------------------------
		// 이 부분 왜 되는지 이해 X
		// Get name from "socket_info_buffer->socket"
		getpeername(socket_info_buffer->socket, (SOCKADDR*)&clinet_address, &addrlen);
		//printf("[테스트]: IP 주소=%s, 포트 번호=%d\n",
		//	inet_ntoa(clinet_address.sin_addr), ntohs(clinet_address.sin_port));
		// -----------------------------------------------------------------------------------------


		// Check async IO result, 종료된 클라이언트를 검출
		if (retval == 0 || transferred_size == 0) {
			if (retval == 0) {
				DWORD temp1, temp2;
				WSAGetOverlappedResult(socket_info_buffer->socket, &socket_info_buffer->overlapped,
					&temp1, FALSE, &temp2);
				err_display((char*)"WSAGetOverlappedResult()");
			}
			closesocket(socket_info_buffer->socket);
			printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
				inet_ntoa(clinet_address.sin_addr), ntohs(clinet_address.sin_port));
			delete socket_info_buffer;
			continue;
		}


		// 받아왔기때문에 여기까지 올 수 있다. 
		CtsPacket read;
		memcpy(&read, socket_info_buffer->buf, sizeof(socket_info_buffer->buf));
		printf("[테스트]: IP 주소=%s, 포트 번호=%d, 내용 : %x\n",
			inet_ntoa(clinet_address.sin_addr), ntohs(clinet_address.sin_port), read.keyboard_click);
		//cout << std::hex << read.keyboard_click << endl;

		DWORD recvbytes;
		DWORD flags = 0;

		retval = WSARecv(socket_info_buffer->socket, &socket_info_buffer->wsa_buffer, 1,
			&recvbytes, &flags, &socket_info_buffer->overlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display((char*)"WSARecv()");
			}
			continue;
		}


		// 받는부분 상시 대기

	}
	return 0;
}

ServerFramework::ServerFramework()
{
	cts_packet = new CtsPacket;
	stc_packet = new StcPacket;

}


ServerFramework::~ServerFramework()
{
	delete cts_packet;
	delete stc_packet;
}

int ServerFramework::CreateWSA() {


}


int ServerFramework::Initialize() {
	// initialize WSA
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return 0;
	}

	cout << "CreateIoCompletionPort...";
	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) {
		cout << "CreateIoCompletionPort 초기화 에러" << endl;
		return 0;
	}
	cout << "Done" << endl;

	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	cout << "Num of Processor : " << (int)sys_info.dwNumberOfProcessors << endl;

	// 쓰레드의 수 만큼 WorkerThread 생성
	// 
	HANDLE handle_thread;
	for (int i = 0; i < (int)sys_info.dwNumberOfProcessors; ++i) {
		handle_thread = CreateThread(NULL, 0, this->WorkerThread, hcp, 0, NULL);
		if (handle_thread == NULL) {
			cout << "CreateThread error" << endl;
			return 1;
		}
		CloseHandle(handle_thread);
	}

	// Initialize listen socket
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket == INVALID_SOCKET) {
		err_quit((char*)"socket()");
	}

	int retval = 0;

	// NO IP limit.
	// ALL IP passed
	// server port is 9000 fixed
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(SERVERPORT);
	retval = bind(listen_socket, (SOCKADDR*)&server_address, sizeof(server_address));
	if (retval == SOCKET_ERROR) {
		err_quit((char*)"bind()");
	}

	// Listen()
	retval = listen(listen_socket, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		err_quit((char*)"listen()");
	}

	return true;
}

int ServerFramework::AcceptClient(UINT client_number) {

	// Use for link IOCP. (buffer)
	SOCKET client_socket_buffer;
	SOCKADDR_IN client_address_buffer;
	DWORD recv_bytes, flags;
	int addrlen = sizeof(client_address_buffer);

	client_socket_buffer = accept(listen_socket, (SOCKADDR*)&client_address_buffer, &addrlen);
	if (client_socket_buffer == INVALID_SOCKET) {
		err_display((char*)"accept()");
	}

	cout << "[TCP 서버] 클라이언트 접속: IP 주소=" << inet_ntoa(client_address_buffer.sin_addr)
		<< ", 포트 번호=" << ntohs(client_address_buffer.sin_port) << endl;

	CreateIoCompletionPort((HANDLE)client_socket_buffer, hcp, client_socket_buffer, 0);

	SocketInfo* socket_info_buffer = new SocketInfo;
	if (socket_info_buffer == NULL) {
		err_display((char*)"Create SocketInfo buffer failed");
		return RUFAIL;
	}
	ZeroMemory(&socket_info_buffer->overlapped, sizeof(socket_info_buffer->overlapped));
	socket_info_buffer->socket = client_socket_buffer;
	socket_info_buffer->wsa_buffer.buf = socket_info_buffer->buf;
	socket_info_buffer->wsa_buffer.len = sizeof(CtsPacket);

	// asynchronous io start
	flags = 0;
	int retval = 0;
	
	retval = WSARecv(client_socket_buffer, &socket_info_buffer->wsa_buffer, 1,
		&recv_bytes, &flags, &socket_info_buffer->overlapped, NULL);
	if (retval == SOCKET_ERROR) {
		if (WSAGetLastError() != ERROR_IO_PENDING) {
			err_display((char*)"WSARecv()");
			return RUFAIL;
		}
	}
	return RUOK;
}

int ServerFramework::Update() {
	// 1. Recv From client -> worker thread do this act
	// 2. Collide Check
	// 3. Send to Client




	// Send to client

	return 0;
}

int ServerFramework::CollideCheck() {
	// 

	return 0;
}