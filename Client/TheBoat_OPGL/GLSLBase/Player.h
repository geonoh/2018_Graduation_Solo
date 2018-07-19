#pragma once
class Player
{
	int hp;
	// Player Pos
	VECTOR3 pos;
	// Player LookVector
	VECTOR3 lookvec;

public:
	VECTOR3 GetPos();
	void SetPos(VECTOR3& input_pos);

	VECTOR3 GetLookvec();
	void SetLookvec(VECTOR3& input_lv);

	int GetHP();
	void SetHP(int hp);

	Player();
	~Player();
};

