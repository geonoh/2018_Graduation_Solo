#include "stdafx.h"
#include "Player.h"

Player::Player()
{
}


Player::~Player()
{
}


void Player::SetPos(VECTOR3& input_pos) {
	pos = input_pos;
}
void Player::SetLookvec(VECTOR3& input_lv) {
	lookvec = input_lv;
}
VECTOR3 Player::GetPos() {
	return pos;
}
VECTOR3 Player::GetLookvec() {
	return lookvec;
}

int Player::GetHP() {
	return hp;
}
void Player::SetHP(int input_hp) {
	hp = input_hp;
}