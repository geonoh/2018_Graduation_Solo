#pragma once
class ServerMgr
{
	WSADATA wsa;
	SOCKET sock;
	SOCKADDR_IN server_addr;
	HWND async_handle;
	WSABUF send_wsabuf;
	WSABUF recv_wsabuf;
	int my_client_id = 0;
	bool first_set_id = true;

	char send_buffer[CLIENT_BUF_SIZE] = { 0 };
	char recv_buffer[CLIENT_BUF_SIZE] = { 0 };

	char packet_buffer[CLIENT_BUF_SIZE] = { 0 };
	DWORD in_packet_size = 0;
	DWORD saved_packet_size = 0;
public:
	void Initialize(HWND& hwnd);
	void ClientError();
	void ReadPacket();
	void SendPacket(int type);
	void ProcessPacket(char* ptr);
	void ErrorDisplay(const char* msg, int err_no);

};