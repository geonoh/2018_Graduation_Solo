#pragma once


class Client
{
private:
	SOCKET sock;
	bool in_use;

	float x, y, z;
	bool mv_foward;
	bool mv_backward;
	bool mv_left;
	bool mv_right;


public:
	void SetPos(float x, float y, float z);
	void SetUse(bool);
	void PressKey(KeyType key_type);
	void ReleaseKey(KeyType key_type);
	Client();
	~Client();
};