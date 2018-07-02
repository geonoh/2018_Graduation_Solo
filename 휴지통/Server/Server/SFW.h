#pragma once


class HeightMap;
class Client;

class SFW
{
private:
	HANDLE iocp_handle;
	SOCKET listen_socket;
	SOCKADDR_IN server_addr;
	HeightMap* height_map;
	Client* clients[4];
public:
	SFW();
	~SFW();
	void Init();
	void AcceptPlayer();
	void WorkerThread();
};

