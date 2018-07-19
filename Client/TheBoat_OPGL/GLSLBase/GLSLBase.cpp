/*
Copyright 2018 Lee Taek Hee (Korea Polytech University)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/

#include "stdafx.h"
#include "Renderer.h"
#include "ServerMgr.h"
#include "Player.h"

#define Dev_
#define INCREASE_HEIGHT	0
#define DECREASE_HEIGHT 10
using namespace std;

Renderer *g_Renderer = NULL;
float gTime = 0.f;

float g_prevX = 0.f;
float g_prevY = 0.f;
float g_accX = 0.f;
float g_accY = 0.f;
int g_LBState = GLUT_UP;

// Camera var
float camera_pos_x = 0.f;
// Cube 그릴때 필요한것
//float camera_pos_y = 0.5f;
//float camera_pos_z = 3.f;
float camera_look_x = 0.f;


// Skybox, Terrain
float camera_pos_y = 30.f;
float camera_pos_z = 0.f;
float camera_look_y = 30.f;
float camera_look_z = 1.f;



// ServerManager
ServerMgr server_mgr;
bool first_recv = true;	// 최초 recv에서 플레이어 정보 받아옴
bool is_connected = false; // 연결 되었는지.


// ServerFunction
void InitSocket();
void NetworkThread();

// Player HP
//int player_hp = 0;
//VECTOR3 player_lookvec = { 0 };
//VECTOR3 player_pos = { 0 };
Player players[NUM_OF_PLAYER];

// Client ID
int client_id = NULL;

// key input
bool keyboard[11] = { 0 };

// gl function
void MouseMove(int x, int y);
void KeyInput(unsigned char key, int x, int y);
void KeyRelease(unsigned char key, int x, int y);
void RenderScene(void);


void Idle(void)
{
	RenderScene();
}

void MouseInput(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		g_LBState = GLUT_DOWN;
		g_prevX = (float)x;
		g_prevY = (float)y;
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		g_LBState = GLUT_UP;
		g_prevX = (float)x;
		g_prevY = (float)y;
	}
	RenderScene();
}



void SpecialKeyInput(int key, int x, int y)
{
	RenderScene();
}

int main(int argc, char **argv)
{
	// 통신
	InitSocket();
	// Initialize GL things
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutCreateWindow("TheBoat Client");
	glewInit();
	if (glewIsSupported("GL_VERSION_3_0"))
	{
		std::cout << " GLEW Version is 3.0\n ";
	}
	else
	{
		std::cout << "GLEW 3.0 not supported\n ";
	}

	// 커서 안보이게 
	//glutSetCursor(GLUT_CURSOR_NONE);



	// Initialize Renderer
	g_Renderer = new Renderer(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!g_Renderer->IsInitialized())
	{
		std::cout << "Renderer could not be initialized.. \n";
	}

	//// 커서 고정
	//RECT rc;
	//rc.left = 0;
	//rc.right = 1000;
	//rc.top = 1000;
	//rc.bottom = 0;
	//ClipCursor(&rc);

	glutDisplayFunc(RenderScene);
	glutIdleFunc(Idle);
	glutKeyboardFunc(KeyInput);
	glutKeyboardUpFunc(KeyRelease);
	glutMouseFunc(MouseInput);
	glutMotionFunc(MouseMove);
	glutPassiveMotionFunc(MouseMove);
	glutSpecialFunc(SpecialKeyInput);

	thread network_thread{ NetworkThread };

	glutMainLoop();

	network_thread.join();
	delete g_Renderer;

    return 0;
}

void InitSocket() {
#ifdef Dev_
	server_mgr.IPInput("127.0.0.1");
#else
	string ip_buf;
	cout << "input ip : ";
	cin >> ip_buf;
	server_mgr.IPInput(ip_buf);
#endif
	// 초기화와 동시에 서버 연결
	is_connected = server_mgr.Initialize();

}

void NetworkThread() {
	if (is_connected) {
		while (true) {
			// ReadPacket인자로 다 때려박는게 나을까?
			server_mgr.ReadPacket();
			// 최초 수신 시 플레이어 번호 할당
			if (first_recv) {
				first_recv = false;
				client_id = server_mgr.ReturnCameraID();
				printf("카메라는 %d에 고정\n", client_id);
			}
			else {
				// 다른 플레이어가 바라보는 방향을 클라이언트에서 반영해야함
				if (server_mgr.GetClientID() != client_id) {
					// 다른 플레이어 SetLook 해줘야함.
					//m_pPlayer[server_mgr.GetClientID()]->SetLook(server_mgr.ReturnLookVector());
					// 이런식으로 
				}
				//m_pPlayer[server_mgr.GetClientID()]->SetPosition(server_mgr.ReturnPlayerPosStatus(server_mgr.GetClientID()).pos);
				// 받은 플레이어 위치를 반영해야한다. 
				server_mgr.GetBullet();

				// 아이템 생성 
				if (server_mgr.IsItemGen())
					server_mgr.ReturnItemPosition();

				players[client_id].SetHP(server_mgr.GetPlayerHP(client_id));
			}
		}
	}
	else {
		printf("---------------------------------\n");
		printf("- Connection Failed. Single Only\n");
		printf("---------------------------------\n");
	}
}

void KeyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case 'w':
		if (keyboard[CS_KEY_PRESS_UP] == true) {
			keyboard[CS_KEY_PRESS_UP] = false;
			server_mgr.SendPacket(CS_KEY_RELEASE_UP);
		}
		break;
	case 'a':
		if (keyboard[CS_KEY_PRESS_LEFT] == true) {
			keyboard[CS_KEY_PRESS_LEFT] = false;
			server_mgr.SendPacket(CS_KEY_RELEASE_LEFT);
		}
		break;
	case 's':
		if (keyboard[CS_KEY_PRESS_DOWN] == true) {
			keyboard[CS_KEY_PRESS_DOWN] = false;
			server_mgr.SendPacket(CS_KEY_RELEASE_DOWN);
		}
		break;
	case 'd':
		if (keyboard[CS_KEY_PRESS_RIGHT] == true) {
			keyboard[CS_KEY_PRESS_RIGHT] = false;
			server_mgr.SendPacket(CS_KEY_RELEASE_RIGHT);
		}
		break;
	case 'q':
		if (keyboard[INCREASE_HEIGHT] == true) {
			keyboard[INCREASE_HEIGHT] = false;
			//server_mgr.SendPacket(CS_KEY_RELEASE_UP, player_lookvec);
		}
		break;
	case 'e':
		if (keyboard[DECREASE_HEIGHT] == true) {
			keyboard[DECREASE_HEIGHT] = false;
			//server_mgr.SendPacket(CS_KEY_RELEASE_UP, player_lookvec);
		}
		break;
	}
}

void KeyInput(unsigned char key, int x, int y)
{
	switch (key) {
	case 'w':
		if (keyboard[CS_KEY_PRESS_UP] == false) {
			keyboard[CS_KEY_PRESS_UP] = true;
			server_mgr.SendPacket(CS_KEY_PRESS_UP);
		}
		camera_pos_z -= 0.1f;
		break;
	case 'a':
		if (keyboard[CS_KEY_PRESS_LEFT] == false) {
			keyboard[CS_KEY_PRESS_LEFT] = true;
			server_mgr.SendPacket(CS_KEY_PRESS_LEFT);
		}
		camera_pos_x -= 0.1f;
		break;
	case 's':
		if (keyboard[CS_KEY_PRESS_DOWN] == false) {
			keyboard[CS_KEY_PRESS_DOWN] = true;
			server_mgr.SendPacket(CS_KEY_PRESS_DOWN);
		}
		camera_pos_z += 0.1f;
		break;
	case 'd':
		if (keyboard[CS_KEY_PRESS_RIGHT] == false) {
			keyboard[CS_KEY_PRESS_RIGHT] = true;
			server_mgr.SendPacket(CS_KEY_PRESS_RIGHT);
		}
		camera_pos_x += 0.1f;
		break;
	case 'q':
		if (keyboard[INCREASE_HEIGHT] == false) {
			keyboard[INCREASE_HEIGHT] = true;
			//server_mgr.SendPacket(CS_KEY_PRESS_UP, player_lookvec);
		}
		camera_pos_y += 0.1f;
		break;
	case 'e':
		if (keyboard[DECREASE_HEIGHT] == false) {
			keyboard[DECREASE_HEIGHT] = true;
			//server_mgr.SendPacket(CS_KEY_PRESS_UP, player_lookvec);
		}
		camera_pos_y -= 0.1f;
		break;
	case VK_ESCAPE:
		delete g_Renderer;
		exit(1);
		break;
	}
	if (key == 'w' || key == 'a' || key == 's' || key == 'd' || key == 'q' || key == 'e') {
		VECTOR3 normalized_lookvec = { 0 };
		g_Renderer->SetCameraPos(camera_pos_x, camera_pos_y, camera_pos_z);
		//g_Renderer->SetCameraLook();
		printf("[Camera] %f, %f, %f \n", camera_pos_x, camera_pos_y, camera_pos_z);
	}
	RenderScene();
}

void MouseMove(int x, int y)
{
	//g_accX += (x - g_prevX) / 50.f;
	//g_accY += (y - g_prevY) / 50.f;
	//g_prevX = (float)x;
	//g_prevY = (float)y;
	// 마우스 움직일때마다 Player의 LookVector을 보내줘야한다. 
	// 카메라의 Lookvec을 조종하므로써 카메라 앵글을 조절한다. 
	//g_Renderer->SetCameraLook(player_lookvec.x, play5er_lookvec.y, player_lookvec.z);

	VECTOR3 normalized_lookvec = { 0 };
	
	float denominator = sqrt(camera_look_x * camera_look_x + camera_look_y * camera_look_y + camera_look_z * camera_look_z);
	float width_ratio = -(camera_pos_x - ((float)x / SCREEN_WIDTH));
	float height_ratio = camera_pos_y - ((float)y / SCREEN_HEIGHT);
	g_Renderer->SetCameraLook(width_ratio / denominator, height_ratio / denominator, 1.f / denominator);


	printf("마우스 좌표 : [%d, %d], 카메라 look vec : [%f, %f] \n", x, y, width_ratio, height_ratio);

	//server_mgr.SendPacket(CS_MOUSE_MOVE, player_lookvec);
	if (g_LBState == GLUT_DOWN) {
		g_accX += (x - g_prevX) / 50.f;
		g_accY += (y - g_prevY) / 50.f;
		g_prevX = (float)x;
		g_prevY = (float)y;
	}
	RenderScene();
}

void RenderScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.f);

	g_Renderer->SetRotation(g_accY, g_accX);

	//g_Renderer->FillAll(0, 0, 0, 0.4);

	// Renderer Test
	//g_Renderer->DrawSTParticle(-1, 0, 1, 0, gTime);
	float centers[] = { -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5 };
	//g_Renderer->Test(centers, gTime);
	//g_Renderer->Radar(centers, gTime);
	//g_Renderer->Bogang();
	//g_Renderer->TextureApp();
	//g_Renderer->TextureAnim();
	//g_Renderer->ProxyGeo();
	//g_Renderer->DrawParticle();
	g_Renderer->DrawSkyBox();
	g_Renderer->DrawHeightMap();
	g_Renderer->Cube();
	//g_Renderer->Test1();

	gTime += 0.005f;

	glutSwapBuffers();
}
