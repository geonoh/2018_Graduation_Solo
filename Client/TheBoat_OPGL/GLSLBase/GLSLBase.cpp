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

#define Dev_

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
float camera_pos_y = 0.5f;
float camera_pos_z = 3.f;

float camera_look_x = 0.f;
float camera_look_y = 0.f;
float camera_look_z = 0.f;

// ServerManager
ServerMgr server_mgr;


void InitSocket();
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
	g_Renderer->Cube();
	//g_Renderer->Test1();

	gTime += 0.005f;

	glutSwapBuffers();
}

void Idle(void)
{
	RenderScene();
}

void MouseInput(int button, int state, int x, int y)
{
	//button : GLUT_DOWN, GLUT_UP
	//state : GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON
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

void MouseMove(int x, int y)
{
	if (g_LBState == GLUT_DOWN)
	{
		g_accX += (x - g_prevX) / 50.f;
		g_accY += (y - g_prevY) / 50.f;
		g_prevX = (float)x;
		g_prevY = (float)y;
	}
	RenderScene();
}

void KeyInput(unsigned char key, int x, int y)
{
	if (key == 'w')
	{
		camera_pos_z -= 0.01f;
	}
	else if (key == 'a') {
		camera_pos_x -= 0.01f;
	}
	else if (key == 's') {
		camera_pos_z += 0.01f;
	}
	else if (key == 'd') {
		camera_pos_x += 0.01f;
	}
	else if (key == 'e') {
		camera_pos_y += 0.01f;
	}
	else if (key == 'q') {
		camera_pos_y -= 0.01f;
	}
	else if (key == VK_ESCAPE) {
		delete g_Renderer;
		exit(1);
	}
	if (key == 'w' || key == 'a' || key == 's' || key == 'd' || key == 'q' || key == 'e') {
		g_Renderer->SetCameraPos(camera_pos_x, camera_pos_y, camera_pos_z);
		printf("[Camera] %f, %f, %f \n", camera_pos_x, camera_pos_y, camera_pos_z);
	}
	RenderScene();
}

void SpecialKeyInput(int key, int x, int y)
{
	RenderScene();
}

int main(int argc, char **argv)
{
	// Ελ½Ε
	InitSocket();
	// Initialize GL things
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(1000, 1000);
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


	// Initialize Renderer
	g_Renderer = new Renderer(1000, 1000);
	if (!g_Renderer->IsInitialized())
	{
		std::cout << "Renderer could not be initialized.. \n";
	}

	glutDisplayFunc(RenderScene);
	glutIdleFunc(Idle);
	glutKeyboardFunc(KeyInput);
	glutMouseFunc(MouseInput);
	glutMotionFunc(MouseMove);
	glutSpecialFunc(SpecialKeyInput);

	glutMainLoop();

	delete g_Renderer;

    return 0;
}

void InitSocket() {
#ifdef _Dev
#else
	server_mgr.IPInput("127.0.0.1");
#endif
	//server_mgr.Initialize();
}