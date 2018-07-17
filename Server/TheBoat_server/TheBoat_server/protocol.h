#pragma once

#define SERVER_IP			127.0.0.1
#define SERVER_PORT			4000
#define MAX_BUFFER_SIZE		4000
#define MAX_PACKET_SIZE		256
#define MAXIMUM_PLAYER		2
#define	WM_SOCKET			WM_USER + 1
#define CLIENT_BUF_SIZE		1024
#define MAX_AMMO_SIZE			30

// 보트 부품
#define MAX_BOAT_ITEM			4
#define MAX_ITEM				12


// 본인 클라이언트 및 서버에서 사용
//#define RUN_SPEED				2.78f
// 위치 테스트용
#define RUN_SPEED				20.78f
#define METER_PER_PIXEL			20
#define WALK_SPEED				1.67f

// Object 갯수 정리 
#define OBJECT_BUILDING			10

// "시간" 보내는 시간
#define TIME_SEND_TIME			1

///////////////////////////////////////////////////
// Server To Client
#define SC_ENTER_PLAYER			1
#define SC_POS					2
#define SC_REMOVE_PLAYER		3
#define SC_PLAYER_LOOKVEC		4
#define SC_BULLET_POS			5	// Bullet Position
#define SC_COLLSION_PB			6	// Collsion Player to Bullet
#define SC_COLLSION_BDP			7	// Building to Player
#define SC_COLLSION_BB			8	// Bullet Building
#define SC_ITEM_GEN				9	// Actually Item gen packet
#define SC_BUILDING_GEN			10
#define SC_GAME_START			11
#define SC_OUT_OF_AMMO			18
#define SC_FULLY_AMMO			19
#define SC_WORLD_TIME			20
///////////////////////////////////////////////////

///////////////////////////////////////////////////
// Event
#define EVT_COLLISION			12
#define EVT_PLAYER_POS_SEND		13
#define EVT_BULLET_GENERATE		14
#define EVT_BULLET_UPDATE		15
#define EVT_BOAT_ITEM_GEN		16
#define EVT_PACKET_RECV			17
#define EVT_SEND_TIME			21
#define EVT_AMMO_ITEM_GEN		22
///////////////////////////////////////////////////

///////////////////////////////////////////////////
// Client To Server
#define CS_KEY_PRESS_UP			1
#define CS_KEY_PRESS_DOWN		2
#define CS_KEY_PRESS_LEFT		3
#define CS_KEY_PRESS_RIGHT		4
#define CS_KEY_PRESS_SPACE		5
#define CS_KEY_PRESS_SHIFT		6
#define CS_KEY_PRESS_1			7
#define CS_KEY_PRESS_2			8
#define CS_LEFT_BUTTON_DOWN		9
#define CS_RIGHT_BUTTON_DOWN	10


#define CS_KEY_RELEASE_UP			11
#define CS_KEY_RELEASE_DOWN			12
#define CS_KEY_RELEASE_LEFT			13
#define CS_KEY_RELEASE_RIGHT		14
#define CS_KEY_RELEASE_SPACE		15
#define CS_KEY_RELEASE_SHIFT		16
#define CS_KEY_RELEASE_1			17
#define CS_KEY_RELEASE_2			18
#define CS_LEFT_BUTTON_UP			19
#define CS_RIGHT_BUTTON_UP			20
#define CS_MOUSE_MOVE				21
#define CS_RELOAD					22

#define CS_PLAYER_READY		100
#define CS_PLAYER_READY_CANCLE 101
#define CS_PLAYER_TEAM_SELECT	102
///////////////////////////////////////////////////


///////////////////////////////////////////////////
// MAP Point
#define MAP_AREA_1		0
#define MAP_AREA_2		1
#define MAP_AREA_3		2
#define MAP_AREA_4		3

#define MAP_POINT1_X	11.92
#define MAP_POINT1_Z	2805.42

#define MAP_POINT2_X	1965.27
#define MAP_POINT2_Z	3821.54

#define MAP_POINT3_X	3768.23
#define MAP_POINT3_Z	1974.05

#define MAP_POINT4_X	2126.64
#define MAP_POINT4_Z	425.85

#define MAP_POINT5_X	27.55
#define MAP_POINT5_Z	1151.17

#define MAP_POINT6_X	1568.99
#define MAP_POINT6_Z	1061.56

#define MAP_POINT7_X	2376.80
#define MAP_POINT7_Z	2028.17

#define MAP_POINT8_X	1568.99
#define MAP_POINT8_Z	3076.07
///////////////////////////////////////////////////



enum GameMode {
	TEAM_MODE, MELEE
};
enum Team {
	NON_TEAM = 0, TEAM_1, TEAM_2, TEAM_3, TEAM_4
};
enum ARWeapons {
	NON_AR = 0
};
enum SubWeapons {
	NON_SUB = 0
};

// 서버->클라
struct SC_PACKET_ENTER_PLAYER {
	BYTE size;
	BYTE type;
	WORD id;
	float x, y, z;
	// 건물 크기 보낼 때만 사용
	float hp;
	float size_x, size_y, size_z;
};

struct SC_PACKET_LOOCVEC {
	BYTE size;
	BYTE type;
	WORD id;
	DirectX::XMFLOAT3 look_vec;
	int player_status;
};

struct SC_PACKET_POS {
	BYTE size;
	BYTE type;
	WORD id;
	int player_status;
	float x, y, z;
};

struct SC_PACKET_COLLISION {
	BYTE size;
	BYTE type;
	WORD client_id;
	float x, y, z;
	float hp;
};

struct SC_PACKET_ITEM_GEN {
	BYTE size;
	BYTE type;
	char item_type;
	float x, y, z;
};

struct SC_PACKET_REMOVE_PLAYER {
	BYTE size;
	BYTE type;
	WORD client_id;
};

struct SC_PACKET_BULLET {
	BYTE size;
	BYTE type;
	WORD id;
	WORD bullet_id;
	DirectX::XMFLOAT3 pos;

	float x, y, z;
};

struct SC_PACKET_START {
	BYTE size;
	BYTE type;
};

struct SC_PACKET_AMMO_O {
	BYTE size;
	BYTE type;
	char ammo;
};

struct SC_PACKET_TIME {
	BYTE size;
	BYTE type;
	//std::chrono::time_point<std::chrono::system_clock> world_time;
	float world_time;
};


// 클라->서버
struct CS_PACKET_BIGGEST {
	BYTE size;
	BYTE type;
	WORD id;
	bool player_in[4];
};

struct CS_PACKET_KEYUP {
	BYTE size;
	BYTE type;
	DirectX::XMFLOAT3 look_vec;
};
struct CS_PACKET_KEYDOWN {
	BYTE size;
	BYTE type;
};
struct CS_PACKET_KEYLEFT {
	BYTE size;
	BYTE type;
};
struct CS_PACKET_KEYRIGHT {
	BYTE size;
	BYTE type;
};
struct CS_PACKET_KEY1 {
	BYTE size;
	BYTE type;
};
struct CS_PACKET_KEY2 {
	BYTE size;
	BYTE type;
};
struct CS_PACKET_KEYSPACE {
	BYTE size;
	BYTE type;
};
struct CS_PACKET_KEYSHIFT {
	BYTE size;
	BYTE type;
};

struct CS_PACKET_MOUSEMOVE {
	BYTE size;
	BYTE type;
};
struct CS_PACKET_LEFTBUTTON {
	BYTE size;
	BYTE type;
};

struct CS_PACKET_READY {
	BYTE size;
	BYTE type;
};
struct CS_PACKET_MODE_SELECT {
	BYTE size;
	BYTE type;
	GameMode game_mode;	// false - Melee
						// true	- Team
};
struct CS_PACKET_TEAM_SELECT {
	BYTE size;
	BYTE type;
	Team team;
};

struct CS_PACKET_LOOK_VECTOR {
	BYTE size;
	BYTE type;
	DirectX::XMVECTOR look_vector;
};

