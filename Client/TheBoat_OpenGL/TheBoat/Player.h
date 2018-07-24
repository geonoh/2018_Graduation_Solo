#pragma once
class Player
{
	float hp;
	VECTOR3 pos;
	VECTOR3 look;
public:
	float GetHP();
	void SetHP(float HP);

	VECTOR3 GetPos();
	void SetPos(float x, float y, float z);

	VECTOR3 GetLook();
	void SetLook(float x, float y, float z);

	Player();
	~Player();
};

