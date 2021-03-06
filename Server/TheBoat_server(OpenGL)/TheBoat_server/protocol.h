#pragma once

#define SERVER_IP			127.0.0.1
#define SERVER_PORT			4000
#define MAX_BUFFER_SIZE		4000
#define MAX_PACKET_SIZE		256
#define MAX_PLAYER			4
#define	WM_SOCKET			WM_USER + 1
#define CLIENT_BUF_SIZE		1024
#define MAX_AMMO			30

// 보트 부품
#define MAX_BOAT_ITEM			4
#define MAX_ITEM				12


// 본인 클라이언트 및 서버에서 사용
//#define RUN_SPEED				2.78f
// 위치 테스트용
#define RUN_SPEED				1.6f
#define METER_PER_PIXEL			20
#define WALK_SPEED				0.8f

// Object 갯수 정리 
#define OBJECT_BUILDING			10

// "시간" 보내는 시간
#define TIME_SEND_TIME			1

// 플레이어의 키 
//#define PLAYER_HEIGHT			5.f
#define OBB_SCALE_PLAYER_X		5.f
#define OBB_SCALE_PLAYER_Y		8.f
#define OBB_SCALE_PLAYER_Z		5.f

// HeightMap에서 사용
// DirectX12 -> OpenGL 상수
#define DX12_TO_OPGL			256.f

// Boat 아이템 생성 시간.
#define ITEM_BOAT_GEN_TIME			120.f
#define ITEM_AMMO_GEN_TIME			30.f
#define PLAYER_HP_UPDATE_TIME		1.f

// 거리 
#define RAD_PLAYER				5.f
#define RAD_ITEM				3.f
#define RAD_AMMO_ITEM			5.f
#define RAD_BULLET				1.f




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
#define SC_COLLISION_TB			9	// Terrain Bullet
#define SC_ITEM_GEN				10	// Actually Item gen packet
#define SC_BUILDING_GEN			11
#define SC_GAME_START			12
#define SC_OUT_OF_AMMO			13
#define SC_FULLY_AMMO			14
#define SC_WORLD_TIME			15
#define SC_AMMO					16
#define SC_PLAYER_READY			17
#define SC_PLAYER_READY_CANCLE	18
#define SC_MODE_TEAM			19
#define SC_MODE_MELEE			20
#define SC_TEAM_RED				21
#define SC_TEAM_BLUE			22
#define SC_BOAT_ITEM_GEN		23
#define SC_PLAYER_HP_UPDATE		34
#define SC_PLAYER_GET_ITEM		35
#define SC_WEATHER_CHANGE		36
#define SC_HIT					37
#define SC_PLAYER_DIE			38
#define SC_RESULT				39
#define SC_ENTER_LOBBY			40
#define SC_AMMO_ITEM_GEN		41
#define SC_STAMINA				42
///////////////////////////////////////////////////

///////////////////////////////////////////////////
// Event
#define EVT_SEND_PACKET			24
#define EVT_RECV_PACKET			25
#define EVT_COLLISION			26	//
#define EVT_PLAYER_POS_SEND		27	//
#define EVT_BULLET_GENERATE		28	//
#define EVT_BULLET_UPDATE		29	//
#define EVT_BOAT_ITEM_GEN		30	//
#define EVT_SEND_TIME			31	//
#define EVT_AMMO_ITEM_GEN		32	//
#define EVT_PLAYER_HP_UPDATE	33
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
#define CS_PLAYER_READY			11

#define CS_PLAYER_READY_CANCLE		12
#define CS_KEY_RELEASE_UP			13
#define CS_KEY_RELEASE_DOWN			14
#define CS_KEY_RELEASE_LEFT			15
#define CS_KEY_RELEASE_RIGHT		16
#define CS_KEY_RELEASE_SPACE		17
#define CS_KEY_RELEASE_SHIFT		18
#define CS_KEY_RELEASE_1			19
#define CS_KEY_RELEASE_2			20
#define CS_LEFT_BUTTON_UP			21
#define CS_RIGHT_BUTTON_UP			22
#define CS_MOUSE_MOVE				23
#define CS_RELOAD					24

#define CS_TEAM_RED					26
#define CS_TEAM_BLUE				27

#define CS_MODE_TEAM				28
#define CS_MODE_MELEE				29
#define CS_DEBUG_TIME				30
#define CS_ASSENBLE_PARTS			31
#define CS_RESTART_GAME				32
#define CS_ENTER_LOBBY				33
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

//struct VECTOR3 {
//	float x, y, z;
//	VECTOR3() {
//
//	}
//	VECTOR3(float ix, float iy, float iz) {
//		x = ix;
//		y = iy;
//		z = iz;
//	}
//};



enum GameMode {
	e_Team, e_Melee
};
enum Team {
	e_NoTeam, e_TeamRed, e_TeamBlue
};

// 서버->클라
struct SC_PACKET_STAMINA {
	BYTE size;
	BYTE type;
	float m_fStamina;
	char m_cID;
};


struct SC_PACKET_ENTER_LOBBY {
	BYTE size;
	BYTE type;

};


struct SC_PACKET_RESULT {
	BYTE size;
	BYTE type;
	char m_cVictoryTeam;
};


struct SC_PACKET_DIE {
	BYTE size;
	BYTE type;
	char m_cDiePlayer;
	float m_fDiePosX;
	float m_fDiePosY;
	float m_fDiePosZ;
	bool m_bBoatItem[4] = { false };
};


struct SC_PACKET_HIT {
	BYTE size;
	BYTE type;
	float m_fHp;
	char m_cShooterID;
	char m_cBulletNumber;
	char m_cHitID;
};

struct SC_PACKET_WEATHER {
	BYTE size;
	BYTE type;
};


struct SC_PACKET_GET_ITEM {
	BYTE size;
	BYTE type;
	char m_cItemType;
	char m_cAmmoItemID;
	char m_cGetterID;
};


struct SC_PACKET_ENTER_PLAYER {
	BYTE size;
	BYTE type;
	WORD id;
	float x, y, z;
	float hp;
	float size_x, size_y, size_z;
	int m_CurrentAmmo;
	int m_TotalAmmo;
	float m_fStamina;
};

struct SC_PACKET_LOOCVEC {
	BYTE size;
	BYTE type;
	WORD id;
	glm::vec3 m_v3LookVec;
};

struct SC_PACKET_POS {
	BYTE size;
	BYTE type;
	WORD id;
	float x, y, z;
	float m_fStamina;
};

struct SC_PACKET_COLLISION_TB {
	BYTE size;
	BYTE type;
	char m_cBulletID;
	char m_cPlayerID;
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
	char m_cItemID;
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
	//DirectX::XMFLOAT3 pos;
	//glm::vec3 pos;
	glm::vec3 pos;
	float x, y, z;
	bool m_bInUse;
};

struct SC_PACKET_START {
	BYTE size;
	BYTE type;
	Team m_bPlayerTeam[MAX_PLAYER] = { e_NoTeam };
};

struct SC_PACKET_AMMO_O {
	BYTE size;
	BYTE type;
	char ammo;
	char m_cTotalAmmo;
};

struct SC_PACKET_AMMO {
	BYTE size;
	BYTE type;
	char m_CurrentAmmo;
	char m_TotalAmmo;
};

struct SC_PACKET_TIME {
	BYTE size;
	BYTE type;
	//std::chrono::time_point<std::chrono::system_clock> world_time;
	float world_time;
};

struct SC_PACKET_GAMEMODE {
	BYTE size;
	BYTE type;

};

struct SC_PACKET_PLAYER_HP_UPDATE {
	BYTE size;
	BYTE type;
	char m_cPlayerID;
	float m_fHp;
};

// 클라->서버
struct CS_PACKET_ASSEMBLE {
	BYTE size;
	BYTE type;
};

struct CS_PACKET_BIGGEST {
	BYTE size;
	BYTE type;
	WORD id;
	bool player_in[4];
};

struct CS_PACKET_KEYUP {
	BYTE size;
	BYTE type;
	//DirectX::XMFLOAT3 m_v3LookVec;
	//glm::vec3 m_v3LookVec;
	glm::vec3 m_v3LookVec;
};
struct CS_PACKET_g_bKeyDown {
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

struct SC_PACKET_READY {
	BYTE size;
	BYTE type;
	char m_cPlayerNumber;
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
	BYTE m_cID;
};

struct CS_PACKET_LOOK_VECTOR {
	BYTE size;
	BYTE type;
	//DirectX::XMVECTOR look_vector;
	//glm::vec3 look_vector;
	glm::vec3 look_vector;
};

