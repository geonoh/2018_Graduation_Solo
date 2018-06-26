#include "stdafx.h"
#include "SFW.h"
#include "HeightMap.h"
#include "Client.h"

SFW::SFW()
{
}


SFW::~SFW()
{
	for (auto d : clients)
		delete d;
	delete height_map;
}

void SFW::AcceptPlayer() {

}

void SFW::Init() {
	
	for (auto d : clients) {
		d = new Client{};
	}
	

	// Windows Socket and IOCP Set
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}

void SFW::WorkerThread() {

}