#pragma once
class Player
{
	float hp;
	glm::vec3 pos;
	glm::vec3 look;
public:
	float GetHP();
	void SetHP(float HP);

	glm::vec3 GetPos();
	void SetPos(float x, float y, float z);

	glm::vec3 GetLook();
	void SetLook(float x, float y, float z);

	Player();
	~Player();
};

