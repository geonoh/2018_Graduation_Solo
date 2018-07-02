#pragma once

static int const MAX_BUFF_SIZE = 4000;

enum SevEvnt {
	evt_recv
};

enum KeyType {
	key_w, key_a, key_s, key_d
};

struct Exover {
	WSAOVERLAPPED wsa_over;
	WSABUF wsabuf;
	
	SevEvnt evnt;
	char io_buf[MAX_BUFF_SIZE];
};