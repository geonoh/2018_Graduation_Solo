#pragma once
class Player
{
	float hp;
	glm::vec3 pos;
	glm::vec3 look;
	int m_CurrentAmmo;
	int m_TotalAmmo;
public:
	bool m_bReady;
	float GetHP();
	void SetHP(float HP);

	glm::vec3 GetPos();
	void SetPos(float x, float y, float z);

	glm::vec3 GetLook();
	void SetLook(float x, float y, float z);

	int GetCurrentAmmo();
	void SetCurrentAmmo(int i_iAmmo);

	int GetTotalAmmo();
	void SetTotalAmmo(int i_iAmmo);


	Player();
	~Player();
};

