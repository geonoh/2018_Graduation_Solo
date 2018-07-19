#include "stdafx.h"
#include "NetworkMgr.h"


NetworkMgr::NetworkMgr()
{
	int retval = 0;

	client_to_server_packet = new CtsPacket;

	// WSA와 listen소켓 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		std::cout << "WSA 초기화 실패" << std::endl;
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);
	//if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
}

void NetworkMgr::ConnectClient() {
	int retval = 0;


	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		std::cout << "소켓 연결 안됬음" << std::endl;
	}
	//if (retval == SOCKET_ERROR) err_quit("connect()");
}


NetworkMgr::~NetworkMgr()
{
	delete client_to_server_packet;
}

void NetworkMgr::ReleaseUPandSend() {
	int retval = 0;
	client_to_server_packet->ReleaseArrowKeyUp();

	retval = send(sock, (char*)client_to_server_packet, sizeof(*client_to_server_packet), 0);
	if (retval == SOCKET_ERROR) {
		std::cout << "보내는거 오류났음" << std::endl;
	}
}
void NetworkMgr::ReleaseDownandSend() {
	int retval = 0;
	client_to_server_packet->ReleaseArrowKeyDown();

	retval = send(sock, (char*)client_to_server_packet, sizeof(*client_to_server_packet), 0);
	if (retval == SOCKET_ERROR) {
		std::cout << "보내는거 오류났음" << std::endl;
	}

}
void NetworkMgr::ReleaseRightandSend() {
	int retval = 0;
	client_to_server_packet->ReleaseArrowKeyRight();

	retval = send(sock, (char*)client_to_server_packet, sizeof(*client_to_server_packet), 0);
	if (retval == SOCKET_ERROR) {
		std::cout << "보내는거 오류났음" << std::endl;
	}

}
void NetworkMgr::ReleaseLeftandSend() {
	int retval = 0;
	client_to_server_packet->ReleaseArrowKeyLeft();

	retval = send(sock, (char*)client_to_server_packet, sizeof(*client_to_server_packet), 0);
	if (retval == SOCKET_ERROR) {
		std::cout << "보내는거 오류났음" << std::endl;
	}

}
void NetworkMgr::PushUPandSend() {
	int retval = 0;
	client_to_server_packet->PushArrowKeyUp();

	retval = send(sock, (char*)client_to_server_packet, sizeof(*client_to_server_packet), 0);
	if (retval == SOCKET_ERROR) {
		std::cout << "보내는거 오류났음" << std::endl;
	}

}
void NetworkMgr::PushDownandSend() {
	int retval = 0;
	client_to_server_packet->PushArrowKeyDown();

	retval = send(sock, (char*)client_to_server_packet, sizeof(*client_to_server_packet), 0);
	if (retval == SOCKET_ERROR) {
		std::cout << "보내는거 오류났음" << std::endl;
	}

}
void NetworkMgr::PushRightandSend() {
	int retval = 0;
	client_to_server_packet->PushArrowKeyRight();

	retval = send(sock, (char*)client_to_server_packet, sizeof(*client_to_server_packet), 0);
	if (retval == SOCKET_ERROR) {
		std::cout << "보내는거 오류났음" << std::endl;
	}

}
void NetworkMgr::PushLeftandSend() {
	int retval = 0;
	client_to_server_packet->PushArrowKeyLeft();

	retval = send(sock, (char*)client_to_server_packet, sizeof(*client_to_server_packet), 0);
	if (retval == SOCKET_ERROR) {
		std::cout << "보내는거 오류났음" << std::endl;
	}

}

