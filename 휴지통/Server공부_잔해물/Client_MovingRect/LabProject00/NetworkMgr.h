#pragma once
class NetworkMgr
{
private:
	CtsPacket* client_to_server_packet;
	WSADATA wsa;
	SOCKET sock;
	SOCKADDR_IN serveraddr;

	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	char recv_ctspacket_buffer[sizeof(CtsPacket)] = { 0 };


public:
	NetworkMgr();
	~NetworkMgr();

	void ReleaseUPandSend();
	void ReleaseDownandSend();
	void ReleaseRightandSend();
	void ReleaseLeftandSend();

	void PushUPandSend();
	void PushDownandSend();
	void PushRightandSend();
	void PushLeftandSend();

	void ConnectClient();

};

