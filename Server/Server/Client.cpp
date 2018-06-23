#include "stdafx.h"
#include "Client.h"


Client::Client()
{
}


Client::~Client()
{
}

void Client::SetPos(float in_x, float in_y, float in_z) {
	x = in_x; 	y = in_y; 	z = in_z;
}

void Client::SetUse(bool ru_use) {
	in_use = ru_use;
}

void Client::PressKey(KeyType key_type) {
	switch (key_type) {
	case key_w:
		mv_foward = true;
		break;
	case key_a:
		mv_left = true;
		break;
	case key_s:
		mv_backward = true;
		break;
	case key_d:
		mv_right = true;
		break;
	}
}

void Client::ReleaseKey(KeyType key_type) {
	switch (key_type) {
	case key_w:
		mv_foward = false;
		break;
	case key_a:
		mv_left = false;
		break;
	case key_s:
		mv_backward = false;
		break;
	case key_d:
		mv_right = false;
		break;
	}
}