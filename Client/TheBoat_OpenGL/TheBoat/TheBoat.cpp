// TheBoat.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "TheBoat.h"
#include "Renderer.h"
#include "ServerMgr.h"
#include "Player.h"
#include "CHeightMapImage.h"

#define MAX_LOADSTRING 100
#define MOUSE_MOVE_SEND 30
// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// WinApi에서 OpenGL을 쓰기위해서는
// 다음과 같은것들이 필요하다. 
//HWND hwnd_main;
//HDC hdc;
//HGLRC hrc;

// OpenGL 
// Renderer
void InitGLRenderer();
Renderer *g_Renderer = nullptr;

// GL Function
void MouseMove(int x, int y);
void KeyInput(unsigned char key, int x, int y);
void KeyRelease(unsigned char key, int x, int y);
void RenderScene(void);
void Idle(void);
void MouseInput(int button, int state, int x, int y);
void SpecialKeyInput(int key, int x, int y);
void SpecialkeyRelease(int key, int x, int y);
void ResetPointer();

HWND g_hwnd;
ServerMgr* g_ServerManager;

// About Client

void InitGame();
int g_iMyID = 0;
bool g_bKeyDown[12] = { false };
Player g_Players[MAX_PLAYER];
Bullet g_Bullets[MAX_PLAYER][MAX_AMMO + 1] = { 0 };
bool g_bFirstRecv = true;;


enum ClientState {
	e_Intro, e_Credit, e_Lobby, e_InGame
};


// Wait Room 
bool g_bTeamMode = false;	// Team or Melee

// 개발용 -> 원래는 e_Intro여야함
//ClientState g_ClientState = e_Intro;
ClientState g_ClientState = e_InGame;
GameMode g_GameMode = e_Team;



bool g_bIntroEnter = true;
bool g_bModeTeam = false;
bool g_bModeMelee = false;
bool g_bTeamRed = false;
bool g_bTeamBlue = false;
bool g_bScope = false;
int g_iMouseMove = 0;

void DrawUI();
void DrawUIAmmo();
void DrawHP();
//-------------------------------------------------
// 높이 정보는 개발용으로만 필요함.
// 어차피 서버에서 받아올거기 때문.
#ifdef _Dev
CHeightMapImage* g_HeightMap = nullptr;
#endif
//-------------------------------------------------




int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.

	// 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_THEBOAT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 응용 프로그램 초기화를 수행합니다.
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_THEBOAT));

	MSG msg;
	InitGLRenderer();

	// 기본 메시지 루프입니다.
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//
	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THEBOAT));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_THEBOAT);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, SCREEN_WIDTH, SCREEN_HEIGHT, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}
	// Winapi 창은 안뜨게 하는걸로.
	//ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		g_hwnd = hWnd;
		break;
	case WM_SOCKET:{
		if (WSAGETSELECTERROR(lParam)) {
			closesocket((SOCKET)wParam);
			g_ServerManager->ClientError();
			break;
		}
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			// 첫번째 읽을때 아이디 저장
			g_ServerManager->ReadPacket();
			if (g_bFirstRecv) {
				g_bFirstRecv = false;
				g_iMyID = g_ServerManager->ReturnCameraID();
				g_Players[g_iMyID].SetTotalAmmo(g_ServerManager->GetTotalAmmo());
				g_Players[g_iMyID].SetCurrentAmmo(g_ServerManager->GetCurrentAmmo());
				printf("카메라는 %d에 고정\n", g_iMyID);
			}

			if (!g_bScope) {
				g_Renderer->SetCameraPos(
					g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.x,
					g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.y,
					g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.z
				);
			}
			for (int i = 0; i < MAX_PLAYER; ++i) {
				g_Players[i].SetPos(
					g_ServerManager->ReturnPlayerPosStatus(i).pos.x,
					g_ServerManager->ReturnPlayerPosStatus(i).pos.y,
					g_ServerManager->ReturnPlayerPosStatus(i).pos.z);
			}
			for (int i = 0; i < MAX_PLAYER; ++i) {
				for (int j = 1; j <= MAX_AMMO; ++j) {
					g_Bullets[i][j] = g_ServerManager->GetBullet(i, j);
				}
			}

			// 아이템생성
			if (g_ServerManager->IsItemGen()) {
				g_ServerManager->ReturnItemPosition();
			}
			// 플레이어 체력	(PlayerNum을 인자로 받음)
			//player_hp = g_ServerManager->GetPlayerHP(g_iMyID);
			g_Players[g_iMyID].SetHP(g_ServerManager->GetPlayerHP(g_iMyID));
			break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			g_ServerManager->ClientError();
			break;
		}
		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 메뉴 선택을 구문 분석합니다.
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		ValidateRect(hWnd, NULL);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_ESCAPE:
			delete g_Renderer;
			delete g_ServerManager;
			PostQuitMessage(0);
			break;
		}
		break;
	
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


void InitGame() {
	glm::vec3 xmf3Scale(8.0f, 2.f, 8.0f);

	LPCTSTR file_name = _T("HeightMap.raw");
#ifdef _Dev
	g_HeightMap = new CHeightMapImage(file_name, 513, 513, xmf3Scale);
	cout << "높이 : " << g_HeightMap->GetHeight(0, 0) << endl;
#endif
	for (int i = 0; i < MAX_PLAYER; ++i) {
		g_Players[i].SetHP(100.f);
		g_Players[i].SetLook(0.f, 0.f, -1.f);
		// HeightMap Add
		//g_HeightMap->GetHeight(10, 10);
#ifdef _Dev
		g_Players[i].SetPos(10.f, g_HeightMap->GetHeight(10, 10), 10.f);
#endif
	}
}


void InitGLRenderer() {
	int argc = 0;
	char **argv = nullptr;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutCreateWindow("TheBoat");
	glewInit();

	//if (GLEW_OK != glewInit())
	//{
	//	printf("GLEW FAIL\n");
	//}
	if (glewIsSupported("GL_VERSION_3_0"))
	{
		std::cout << " GLEW Version is 3.0\n ";
	}
	else
	{
		std::cout << "GLEW 3.0 not supported\n ";
	}

	InitGame();

	g_Renderer = new Renderer(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!g_Renderer->IsInitialized())
	{
		std::cout << "Renderer could not be initialized.. \n";
	}

	g_ServerManager = new ServerMgr;

#ifdef _IP
	g_ServerManager->IPInput();
#else
	g_ServerManager->IPInput("127.0.0.1");
#endif
	if (!g_ServerManager->Initialize(g_hwnd)) {
		printf("---------------------------------\n");
		printf("- Server Initialize Err\n");
		printf("---------------------------------\n");
	}



	glutDisplayFunc(RenderScene);
	glutIdleFunc(Idle);
	glutKeyboardFunc(KeyInput);
	glutKeyboardUpFunc(KeyRelease);
	glutMouseFunc(MouseInput);
	glutMotionFunc(MouseMove);
	glutPassiveMotionFunc(MouseMove);
	glutSpecialFunc(SpecialKeyInput);
	glutSpecialUpFunc(SpecialkeyRelease);
	glutMainLoop();
}

void RenderScene(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.f);

	switch (g_ClientState) {
	case e_Intro:
		g_Renderer->DrawIntro(g_bIntroEnter);
		break;
	case e_Lobby:
		g_Renderer->DrawLobby(0, 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT);
		// TeamMode에서 Red, Blue
		if (g_bModeTeam) {
			g_Renderer->DrawLobby(3, 320.f, 360.f, 320.f, 90.f);
			if (g_bTeamRed) {
				g_Renderer->DrawLobby(3, 320.f, 540.f, 320.f, 90.f);
			}
			if (g_bTeamBlue) {
				g_Renderer->DrawLobby(3, 320.f, 630.f, 320.f, 90.f);
			}

			// Team에 따라 플레이어 주위 색상 변경
			if (g_Players[0].m_bTeam) {
				g_Renderer->DrawLobby(1, 0.f, 0.f, 640.f, 180.f);
			}
			else {
				g_Renderer->DrawLobby(2, 0.f, 0.f, 640.f, 180.f);
			}
			if (g_Players[1].m_bTeam) {
				g_Renderer->DrawLobby(1, 640.f, 0.f, 640.f, 180.f);
			}
			else {
				g_Renderer->DrawLobby(2, 640.f, 0.f, 640.f, 180.f);
			}
			if (g_Players[2].m_bTeam) {
				g_Renderer->DrawLobby(1, 0.f, 180.f, 640.f, 180.f);
			}
			else {
				g_Renderer->DrawLobby(2, 0.f, 180.f, 640.f, 180.f);
			}
			if (g_Players[3].m_bTeam) {
				g_Renderer->DrawLobby(1, 640.f, 180.f, 640.f, 180.f);
			}
			else {
				g_Renderer->DrawLobby(2, 640.f, 180.f, 640.f, 180.f);
			}
		}
		// Melee에 동그라미 쳐주기 
		if (g_bModeMelee) {
			g_Renderer->DrawLobby(3, 320.f, 450.f, 320.f, 90.f);
		}
		// MyID에 따라 플레이어 주위 노란색 Box
		switch (g_iMyID) {
		case 0:
			g_Renderer->DrawLobby(4, 0.f, 0.f, 640.f, 180.f);
			break;
		case 1:
			g_Renderer->DrawLobby(4, 640.f, 0.f, 640.f, 180.f);
			break;
		case 2:
			g_Renderer->DrawLobby(4, 0.f, 180.f, 640.f, 180.f);
			break;
		case 3:
			g_Renderer->DrawLobby(4, 640.f, 180.f, 640.f, 180.f);
			break;
		}
		if (g_Players[0].m_bReady) {
			g_Renderer->DrawLobby(5, 540.f, 0.f, 100.f, 180.f);
		}
		if (g_Players[1].m_bReady) {
			g_Renderer->DrawLobby(5, 1180, 0.f, 100.f, 180.f);
		}
		if (g_Players[2].m_bReady) {
			g_Renderer->DrawLobby(5, 540.f, 180.f, 100.f, 180.f);
		}
		if (g_Players[3].m_bReady) {
			g_Renderer->DrawLobby(5, 1180, 180.f, 100.f, 180.f);
		}

		// 플레이어 레디 했는지.
		if (g_Players[0].m_bReady) {
			g_Renderer->DrawLobby(5, 0.f, 0.f, 640.f, 180.f);
		}
		if (g_Players[1].m_bReady) {
			g_Renderer->DrawLobby(5, 640.f, 0.f, 640.f, 180.f);
		}
		if (g_Players[2].m_bReady) {
			g_Renderer->DrawLobby(5, 0.f, 180.f, 640.f, 180.f);
		}
		if (g_Players[3].m_bReady) {
			g_Renderer->DrawLobby(5, 640.f, 180.f, 640.f, 180.f);
		}


		break;
	case e_Credit:
		g_Renderer->DrawCredit();
		break;
	case e_InGame:
		g_Renderer->DrawSkyBox();
		g_Renderer->DrawHeightMap();

		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (g_iMyID == i) {
				g_Players[i].SetLook(g_Renderer->GetCameraLook().x,
					g_Renderer->GetCameraLook().y,
					g_Renderer->GetCameraLook().z);
				g_Renderer->DrawCube(g_Players[i].GetPos().x, g_Players[i].GetPos().y, g_Players[i].GetPos().z);
				//printf("%d 플레이어의 LookVec [%f, %f, %f] \n", i,
				//	g_Players[i].GetLook().x,
				//	g_Players[i].GetLook().y,
				//	g_Players[i].GetLook().z);
			}
			else {
				g_Players[i].SetLook(g_ServerManager->ReturnLookVector(i).x,
					g_ServerManager->ReturnLookVector(i).y,
					g_ServerManager->ReturnLookVector(i).z);
				//printf("%d 플레이어의 LookVec [%f, %f, %f] \n", i,
				//	g_Players[i].GetLook().x,
				//	g_Players[i].GetLook().y,
				//	g_Players[i].GetLook().z);
				g_Renderer->DrawCube(g_Players[i].GetPos().x, g_Players[i].GetPos().y, g_Players[i].GetPos().z,
					g_Players[i].GetLook().x, g_Players[i].GetLook().y, g_Players[i].GetLook().z);
			}
		}
		// Draw Bullet
		for (int i = 0; i < MAX_PLAYER; ++i) {
			for (int j = 1; j <= MAX_AMMO; ++j) {
				if (g_Bullets[i][j].in_use) {
					g_Renderer->DrawBullet(g_Bullets[i][j].x, g_Bullets[i][j].y, g_Bullets[i][j].z);
				}
			}
		}

		g_Renderer->DrawParticle(0.02f);
		DrawUI();
		break;
	}



	glutSwapBuffers();
}

void DrawUIAmmo() {
	// Total Ammo Count
	// 10의자리 
	g_Players[g_iMyID].SetTotalAmmo(g_ServerManager->GetTotalAmmo());
	g_Players[g_iMyID].SetCurrentAmmo(g_ServerManager->GetCurrentAmmo());


	int nTens = g_Players[g_iMyID].GetTotalAmmo() / 10;
	int nUnits = g_Players[g_iMyID].GetTotalAmmo() % 10;

	// 10의자리가 0이 아닐경우에만 십의자리를 그림
	if (nTens != 0) {
		g_Renderer->DrawUITexture(nTens,
			160.f, SCREEN_HEIGHT - 80.f,
			50.f, 50.f);
	}
	g_Renderer->DrawUITexture(nUnits,
		240.f, SCREEN_HEIGHT - 80.f,
		50.f, 50.f);

	// 그 사이 Slash
	g_Renderer->DrawUITexture(12,
		120.f, SCREEN_HEIGHT - 80.f,
		50.f, 50.f);



	// Current Ammo Count
	// 10의자리 
	int nCurrentAmmoTens = g_Players[g_iMyID].GetCurrentAmmo() / 10;
	int nCurrentAmmoUnits = g_Players[g_iMyID].GetCurrentAmmo() % 10;

	// 10의자리가 0이 아닐경우에만 십의자리를 그림
	if (nCurrentAmmoTens != 0) {
		g_Renderer->DrawUITexture(nCurrentAmmoTens,
			0.f, SCREEN_HEIGHT - 80.f,
			50.f, 50.f);
	}
	g_Renderer->DrawUITexture(nCurrentAmmoUnits,
		80.f, SCREEN_HEIGHT - 80.f,
		50.f, 50.f);



}

void DrawUI() {
	// PlayerPos
	//printf("Player Pos : [%f, %f, %f] \n",
	//	g_Players[g_iMyID].GetPos().x,
	//	g_Players[g_iMyID].GetPos().y,
	//	g_Players[g_iMyID].GetPos().z);

	// DrawMiniMap
	g_Renderer->DrawUITexture(10,
		SCREEN_WIDTH - SIZE_MINIMAP, SCREEN_HEIGHT - SIZE_MINIMAP,
		SIZE_MINIMAP, SIZE_MINIMAP);

	// Draw Pin
	float fPlayerPosX = g_Players[g_iMyID].GetPos().x / 256.f;
	float fPlayerPosY = -g_Players[g_iMyID].GetPos().z / 256.f;
	fPlayerPosX += 1.f;
	fPlayerPosY += 1.f;

	fPlayerPosX *= SIZE_MINIMAP / 2;
	fPlayerPosY *= SIZE_MINIMAP / 2;
	g_Renderer->DrawUITexture(11,
		SCREEN_WIDTH - SIZE_MINIMAP + fPlayerPosX, SCREEN_HEIGHT - SIZE_MINIMAP + fPlayerPosY,
		50.f, 50.f);

	// Draw Timer Bar
	float fDrawPoint = g_ServerManager->GetTime() / (ITEM_BOAT_GEN_TIME * 1000);
	// TimerBackground
	g_Renderer->DrawUITexture(13,
		1000.f, 400.f,
		280.f, 40.f);
	// DrawProgressBar
	g_Renderer->DrawUITexture(14,
		1000.f, 400.f,
		(fDrawPoint) * 280.f, 40);
	g_Renderer->DrawUITexture(19,
		1000.f, 360.f,
		280.f, 40.f);



	DrawUIAmmo();
	DrawHP();

	// DrawScope
	if (g_bScope) {
		g_Renderer->GetCameraLook();
		//g_Renderer->SetCameraPos(
		//	g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.x,
		//	g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.y,
		//	g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.z
		//);

		g_Renderer->DrawUITexture(15,
			0.f, 0.f,
			SCREEN_WIDTH, SCREEN_HEIGHT);
	}
	//else {
	//	// Draw CrossHair
	//	g_Renderer->DrawUITexture(16,
	//		0.f, 0.f,
	//		SCREEN_WIDTH, SCREEN_HEIGHT);

	//	g_Renderer->SetCameraPos(
	//		g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.x,
	//		g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.y,
	//		g_ServerManager->ReturnPlayerPosStatus(g_iMyID).pos.z
	//	);
	//}


}

void DrawHP() {
	//printf("player hp : %d \n", g_Players[g_iMyID].GetHP());

	// 체력 반영해서 그리기 ( Scale이용해서 그리면 됨)
	float fHpPercent = g_Players[g_iMyID].GetHP() / 100.f;
	g_Renderer->DrawUITexture(18,
		490.f,608.f,
		fHpPercent * 350, 80);
	// 디폴트 체력 바 그리기
	g_Renderer->DrawUITexture(17,
		0.f, 0.f,
		SCREEN_WIDTH, SCREEN_HEIGHT);
	

}


void Idle(void)
{
	// 상시 업데이트 해주기 
	g_Renderer->UpdateView();
	for (int i = 0; i < MAX_PLAYER; ++i) {
		g_Players[i].m_bReady = g_ServerManager->GetPlayerReadyStatus()[i];
	}
	if (g_ServerManager->GetGameMode()) {
		g_bModeTeam = false;
		g_bModeMelee = true;
	}
	else {
		g_bModeTeam = true;
		g_bModeMelee = false;
		// 
		for (int i = 0; i < MAX_PLAYER; ++i) {
			g_Players[i].m_bTeam = g_ServerManager->GetTeam()[i];
		}
	}
	if (g_ServerManager->GetStart()) {
		g_ClientState = e_InGame;
	}

	RenderScene();
}

void MouseInput(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (g_ClientState == e_InGame) {
			printf("Ammo : %d\n", g_ServerManager->GetCurrentAmmo());
			g_ServerManager->SendPacket(CS_LEFT_BUTTON_DOWN, g_Renderer->GetCameraLook());
			g_Players[g_iMyID].SetCurrentAmmo(g_ServerManager->GetCurrentAmmo());
		}
		else if (g_ClientState == e_Credit) {
			if (1005 <= x && x <= 1210) {
				if (38 <= y && y <= 131) {
					g_ClientState = e_Intro;
				}
			}
		}
		else  if (g_ClientState == e_Intro) {
			if (528 <= x && x <= 827) {
				if (369 <= y && y <= 486) {
					// Entering Lobby
					g_ClientState = e_Lobby;
				}
				if (500 <= y && y <= 600) {
					// Entering Credit
					g_ClientState = e_Credit;
				}
			}
		}
		else if (g_ClientState == e_Lobby) {
			if (320 <= x && x <= 640) {
				if (g_iMyID == 0) {
					if (360 <= y && y <= 450) {
						g_bModeTeam = true;
						g_bModeMelee = false;
						g_ServerManager->SendPacket(CS_MODE_TEAM);
					}
					if (450 <= y && y <= 540) {
						g_bModeTeam = false;
						g_bModeMelee = true;
						g_bTeamRed = false;
						g_bTeamBlue = false;
						g_ServerManager->SendPacket(CS_MODE_MELEE);
					}
				}
				if (g_bModeTeam) {
					if (540 <= y && y <= 630) {
						g_bTeamRed = true;
						g_bTeamBlue = false;
						g_Players[g_iMyID].m_bTeam = false;
						g_ServerManager->SendPacket(CS_TEAM_RED);
					}
					if (630 <= y && y <= 720) {
						g_bTeamRed = false;
						g_bTeamBlue = true;
						g_Players[g_iMyID].m_bTeam = true;
						g_ServerManager->SendPacket(CS_TEAM_BLUE);
					}
				}
			}
			if (640 <= x && x <= 1280) {
				if (540 <= y && y <= 720) {
					if (g_Players[g_iMyID].m_bReady == true) {
						g_Players[g_iMyID].m_bReady = false;
						g_ServerManager->SendPacket(CS_PLAYER_READY_CANCLE);
					}
					else if (g_Players[g_iMyID].m_bReady == false) {
						g_Players[g_iMyID].m_bReady = true;
						g_ServerManager->SendPacket(CS_PLAYER_READY);
					}
					
				}
			}
		}

	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)	{
		if (g_ClientState == e_InGame) {
			g_ServerManager->SendPacket(CS_LEFT_BUTTON_UP);
		}
		else {

		}
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		if (g_ClientState == e_InGame) {
			g_bScope = true;
		}
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
		if (g_ClientState == e_InGame) {
			g_bScope = false;
		}
	}

	RenderScene();
}



void MouseMove(int x, int y)
{
	//printf("마우스 좌표 : [%d, %d]\n", x, y);
	g_Renderer->MouseMove(x, y, SCREEN_WIDTH, SCREEN_HEIGHT, g_ClientState == e_InGame);

	if (g_ClientState == e_InGame) {
		g_iMouseMove++;
		if (g_iMouseMove > MOUSE_MOVE_SEND) {
			g_ServerManager->SendPacket(CS_MOUSE_MOVE, g_Renderer->GetCameraLook());
			g_iMouseMove = 0;
		}
		if (g_bKeyDown[CS_KEY_PRESS_UP] || g_bKeyDown[CS_KEY_PRESS_LEFT] || g_bKeyDown[CS_KEY_PRESS_RIGHT] || g_bKeyDown[CS_KEY_PRESS_DOWN]) {
			g_ServerManager->SendPacket(CS_MOUSE_MOVE, g_Renderer->GetCameraLook());
		}
	}
	else if(g_ClientState == e_Intro){
		if (528 <= x && x <= 827) {
			if (369 <= y && y <= 486) {
				g_bIntroEnter = true;
			}
			if (500 <= y && y <= 600) {
				g_bIntroEnter = false;
			}
		}
	}
	RenderScene();
}

void KeyInput(unsigned char key, int x, int y) {
	g_Renderer->KeyPressed(key);
	if (g_ClientState == e_InGame && g_bScope == false) {
		switch (key) {
		case '1':
			if (g_bKeyDown[CS_KEY_PRESS_1] == false) {
				g_bKeyDown[CS_KEY_PRESS_1] = true;
				g_ServerManager->SendPacket(CS_KEY_PRESS_1);
			}
			break;
		case '2':
			if (g_bKeyDown[CS_KEY_PRESS_2] == false) {
				g_bKeyDown[CS_KEY_PRESS_2] = true;
				g_ServerManager->SendPacket(CS_KEY_PRESS_2);
			}
			break;
			// Debug Key
		case '4':
			printf("%d Player Pos [%f, %f, %f]\n", g_iMyID,
				g_Players[g_iMyID].GetPos().x,
				g_Players[g_iMyID].GetPos().y,
				g_Players[g_iMyID].GetPos().z);
			break;
			// Debug Key
			// 아이템 생성 빠르게 
		case '5':
			g_ServerManager->SendPacket(CS_DEBUG_TIME);
			break;
		case 'W':
		case 'w':
			if (g_bKeyDown[CS_KEY_PRESS_UP] == false) {
				g_bKeyDown[CS_KEY_PRESS_UP] = true;
				g_ServerManager->SendPacket(CS_KEY_PRESS_UP, g_Renderer->GetCameraLook());
			}
			break;
		case 'a':
		case 'A':
			if (g_bKeyDown[CS_KEY_PRESS_LEFT] == false) {
				g_bKeyDown[CS_KEY_PRESS_LEFT] = true;
				g_ServerManager->SendPacket(CS_KEY_PRESS_LEFT, g_Renderer->GetCameraLook());
			}
			break;
		case 's':
		case 'S':
			if (g_bKeyDown[CS_KEY_PRESS_DOWN] == false) {
				g_bKeyDown[CS_KEY_PRESS_DOWN] = true;
				g_ServerManager->SendPacket(CS_KEY_PRESS_DOWN, g_Renderer->GetCameraLook());
			}
			break;
		case 'D':
		case 'd':
			if (g_bKeyDown[CS_KEY_PRESS_RIGHT] == false) {
				g_bKeyDown[CS_KEY_PRESS_RIGHT] = true;
				g_ServerManager->SendPacket(CS_KEY_PRESS_RIGHT, g_Renderer->GetCameraLook());
			}
			break;
		case 'r':
		case 'R':
			g_ServerManager->SendPacket(CS_RELOAD);
			// 여기서 그리던거 전부 초기화 해야한다. 

			break;
		case VK_ESCAPE:
#ifdef _Dev
			delete g_HeightMap;
#endif
			delete g_ServerManager;
			delete g_Renderer;
			exit(1);
			break;
		}
	}
}
void KeyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case '1':
		if (g_bKeyDown[CS_KEY_PRESS_1] == true) {
			g_ServerManager->SendPacket(CS_KEY_RELEASE_1);
			g_bKeyDown[CS_KEY_PRESS_1] = false;
		}
		break;
	case '2':
		if (g_bKeyDown[CS_KEY_PRESS_2] == true) {
			g_ServerManager->SendPacket(CS_KEY_RELEASE_2);
			g_bKeyDown[CS_KEY_PRESS_2] = false;
		}
		break;
	case 'W':
	case 'w':
		if (g_bKeyDown[CS_KEY_PRESS_UP] == true) {
			g_ServerManager->SendPacket(CS_KEY_RELEASE_UP);
			g_bKeyDown[CS_KEY_PRESS_UP] = false;
		}
		break;
	case 'a':
	case 'A':
		if (g_bKeyDown[CS_KEY_PRESS_LEFT] == true) {
			g_ServerManager->SendPacket(CS_KEY_RELEASE_LEFT);
			g_bKeyDown[CS_KEY_PRESS_LEFT] = false;
		}
		break;
	case 's':
	case 'S':
		if (g_bKeyDown[CS_KEY_PRESS_DOWN] == true) {
			g_ServerManager->SendPacket(CS_KEY_RELEASE_DOWN);
			g_bKeyDown[CS_KEY_PRESS_DOWN] = false;
		}
		break;
	case 'D':
	case 'd':
		if (g_bKeyDown[CS_KEY_PRESS_RIGHT] == true) {
			g_ServerManager->SendPacket(CS_KEY_RELEASE_RIGHT);
			g_bKeyDown[CS_KEY_PRESS_RIGHT] = false;
		}
		break;
	}

}
void SpecialKeyInput(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_UP:
		printf("야 위냐\n");
		break;
	case GLUT_KEY_DOWN:
		break;

	case GLUT_KEY_SHIFT_L:
		if (g_bKeyDown[CS_KEY_PRESS_SHIFT] == false) {
			g_ServerManager->SendPacket(CS_KEY_PRESS_SHIFT);
			g_bKeyDown[CS_KEY_PRESS_SHIFT] = true;
		}
		break;
	case GLUT_KEY_F5: 
		if (g_bKeyDown[CS_PLAYER_READY] == false) {
			g_ServerManager->SendPacket(CS_PLAYER_READY);
			g_bKeyDown[CS_PLAYER_READY] = true;
		}
		else if (g_bKeyDown[CS_PLAYER_READY] == true) {
			g_ServerManager->SendPacket(CS_PLAYER_READY_CANCLE);
			g_bKeyDown[CS_PLAYER_READY] = false;
		}
		break;

	}
}

void SpecialkeyRelease(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_SHIFT_L:
		if (g_bKeyDown[CS_KEY_PRESS_SHIFT] == true) {
			g_ServerManager->SendPacket(CS_KEY_RELEASE_SHIFT);
			g_bKeyDown[CS_KEY_PRESS_SHIFT] = false;
		}
		break;
	case GLUT_KEY_F5:
		break;
	}
}

void ResetPointer() {
	glutWarpPointer(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

}

