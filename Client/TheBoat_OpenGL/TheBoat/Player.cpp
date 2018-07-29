#include "stdafx.h"
#include "Player.h"


Player::Player()
{
}


Player::~Player()
{
}

float Player::GetHP() {
	return hp;
}
void Player::SetHP(float i_HP) {
	hp = i_HP;
}

glm::vec3 Player::GetPos() {
	return pos;
}
void Player::SetPos(float x, float y, float z) {
	glm::vec3 i_pos{ x,y,z };
	pos = i_pos;
}

glm::vec3 Player::GetLook() {
	return look;
}
void Player::SetLook(float x, float y, float z) {
	glm::vec3 i_look{ x,y,z };
	look = i_look;
}
