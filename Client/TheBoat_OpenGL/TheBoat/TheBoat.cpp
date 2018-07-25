// TheBoat.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "TheBoat.h"
#include "Renderer.h"
#include "ServerMgr.h"
#include "Player.h"
#include "CHeightMapImage.h"

#define MAX_LOADSTRING 100

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
bool rendering_start = false;
float gTime = 0.f;

// GL Function
void MouseMove(int x, int y);
void KeyInput(unsigned char key, int x, int y);
void KeyRelease(unsigned char key, int x, int y);
void RenderScene(void);
void Idle(void);
void MouseInput(int button, int state, int x, int y);
void SpecialKeyInput(int key, int x, int y);
void SpecialkeyRelease(int key, int x, int y);


// GL global var
//float g_prevX = 0.f;
//float g_prevY = 0.f;
float g_accX = 0.f;
float g_accY = 0.f;
int g_LBState = GLUT_UP;
HWND g_hwnd;

// Network
ServerMgr* server_manager;
bool first_recv = true;;

// About Client

void InitGame();
int my_client_id = 0;
bool keydown[12] = { false };
Player players[MAXIMUM_PLAYER];

// 높이 정보는 개발용으로만 필요함.
// 어차피 서버에서 받아올거기 때문.
#ifdef _Dev
CHeightMapImage* height_map = nullptr;
#endif


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

	server_manager = new ServerMgr;

#ifdef _IP
	server_manager->IPInput();
#else
	server_manager->IPInput("127.0.0.1");
#endif
	if (!server_manager->Initialize(g_hwnd)) {
		printf("---------------------------------\n");
		printf("- Server Initialize Err\n");
		printf("---------------------------------\n");
	}

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		g_hwnd = hWnd;
		//SetTimer(g_hwnd, 1, 1000, NULL);
		break;
	case WM_TIMER:
		switch (wParam) {
		case 1:
			printf("1번 타이머 호출 \n");
			break;
		case 2:
			printf("2번 타이머 호출 \n");
			break;
		}
		break;
	case WM_SOCKET:{
		if (WSAGETSELECTERROR(lParam)) {
			closesocket((SOCKET)wParam);
			server_manager->ClientError();
			break;
		}
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			// 첫번째 읽을때 아이디 저장
			server_manager->ReadPacket();
			if (first_recv) {
				first_recv = false;
				my_client_id = server_manager->ReturnCameraID();
				printf("카메라는 %d에 고정\n", my_client_id);
			}

			// -----------------------------------------------------------
			// 룩벡터 Setting 필요
			if (server_manager->GetClientID() != my_client_id) {
				//m_pPlayer[server_mgr.GetClientID()]->SetLook(server_mgr.ReturnLookVector());
			}
			// -----------------------------------------------------------

			// -----------------------------------------------------------
			// 플레이어 위치 Set해줘야한다.
			//g_Renderer->SetCameraPos();
			//printf("Player %d Pos [%f, %f, %f] \n",
			//	server_manager->GetClientID(),
			//	server_manager->ReturnPlayerPosStatus(server_manager->GetClientID()).pos.x,
			//	server_manager->ReturnPlayerPosStatus(server_manager->GetClientID()).pos.y,
			//	server_manager->ReturnPlayerPosStatus(server_manager->GetClientID()).pos.z);
			g_Renderer->SetCameraPos(
				server_manager->ReturnPlayerPosStatus(server_manager->GetClientID()).pos.x,
				server_manager->ReturnPlayerPosStatus(server_manager->GetClientID()).pos.y,
				server_manager->ReturnPlayerPosStatus(server_manager->GetClientID()).pos.z
				);
			// 아래는 DirectX 코드 
			//m_pPlayer[server_mgr.GetClientID()]->SetPosition(server_mgr.ReturnPlayerPosStatus(server_mgr.GetClientID()).pos);
			//printf("상태 : %d\n",server_mgr.ReturnPlayerPosStatus(server_mgr.GetClientID()).player_status);
			// -----------------------------------------------------------


			// 아이템생성
			if (server_manager->IsItemGen()) {
				server_manager->ReturnItemPosition();
			}
			// 플레이어 체력	(PlayerNum을 인자로 받음)
			//player_hp = server_manager->GetPlayerHP(my_client_id);
			players[my_client_id].SetHP(server_manager->GetPlayerHP(my_client_id));

			break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			server_manager->ClientError();
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
			delete server_manager;
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

	LPCTSTR file_name = _T("height_map.raw");
#ifdef _Dev
	height_map = new CHeightMapImage(file_name, 513, 513, xmf3Scale);
	cout << "높이 : " << height_map->GetHeight(10, 10) << endl;
#endif
	for (int i = 0; i < MAXIMUM_PLAYER; ++i) {
		players[i].SetHP(100.f);
		players[i].SetLook(0.f, 0.f, -1.f);
		// HeightMap Add
		//height_map->GetHeight(10, 10);
#ifdef _Dev
		players[i].SetPos(10.f, height_map->GetHeight(10, 10), 10.f);
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
	//g_Renderer->Cube();
	//g_Renderer->Test1();

	gTime += 0.005f;

	glutSwapBuffers();

}


void Idle(void)
{
	g_Renderer->UpdateView();
	RenderScene();
}

void MouseInput(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (server_manager->GetAmmo() > 0) {
			server_manager->DecreaseAmmo();
			server_manager->SendPacket(CS_LEFT_BUTTON_DOWN);
			printf("Ammo : %d\n", server_manager->GetAmmo());

			//g_LBState = GLUT_DOWN;
			//g_prevX = (float)x;
			//g_prevY = (float)y;
		}
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)	{
		server_manager->SendPacket(CS_LEFT_BUTTON_UP);
		//g_LBState = GLUT_UP;
		//g_prevX = (float)x;
		//g_prevY = (float)y;
	}
	RenderScene();
}



void MouseMove(int x, int y)
{
	//printf("마우스 좌표 : [%d, %d]\n", x, y);
	g_Renderer->MouseMove(x, y, SCREEN_WIDTH, SCREEN_HEIGHT);
	server_manager->SendPacket(CS_MOUSE_MOVE, g_Renderer->GetCameraLook());
	//if (g_LBState == GLUT_DOWN) {
	//	//printf("눌렀다\n");
	//	g_accX += (x - g_prevX) / 50.f;
	//	g_accY += (y - g_prevY) / 50.f;
	//	g_prevX = (float)x;
	//	g_prevY = (float)y;
	//}
	RenderScene();
}

void KeyInput(unsigned char key, int x, int y) {
	g_Renderer->KeyPressed(key);
	switch (key) {
	case '1':
		if (keydown[CS_KEY_PRESS_1] == false) {
			keydown[CS_KEY_PRESS_1] = true;
			server_manager->SendPacket(CS_KEY_PRESS_1);
		}
		break;
	case '2':
		if (keydown[CS_KEY_PRESS_2] == false) {
			keydown[CS_KEY_PRESS_2] = true;
			server_manager->SendPacket(CS_KEY_PRESS_2);
		}
		break;
	case 'W':
	case 'w':
		if (keydown[CS_KEY_PRESS_UP] == false) {
			keydown[CS_KEY_PRESS_UP] = true;
			server_manager->SendPacket(CS_KEY_PRESS_UP);
		}
		break;
	case 'a':
	case 'A':
		if (keydown[CS_KEY_PRESS_LEFT] == false) {
			keydown[CS_KEY_PRESS_LEFT] = true;
			server_manager->SendPacket(CS_KEY_PRESS_LEFT);
		}
		break;
	case 's':
	case 'S':
		if (keydown[CS_KEY_PRESS_DOWN] == false) {
			keydown[CS_KEY_PRESS_DOWN] = true;
			server_manager->SendPacket(CS_KEY_PRESS_DOWN);
		}
		break;
	case 'D':
	case 'd':
		if (keydown[CS_KEY_PRESS_RIGHT] == false) {
			keydown[CS_KEY_PRESS_RIGHT] = true;
			server_manager->SendPacket(CS_KEY_PRESS_RIGHT);
		}
		break;
	case 'r':
	case 'R':
		printf("Reload\n");
		server_manager->SendPacket(CS_RELOAD);
		break;
	case VK_ESCAPE:
#ifdef _Dev
		delete height_map;
#endif
		delete server_manager;
		delete g_Renderer;
		exit(1);
		break;
	}
}
void KeyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case '1':
		if (keydown[CS_KEY_PRESS_1] == true) {
			server_manager->SendPacket(CS_KEY_RELEASE_1);
			keydown[CS_KEY_PRESS_1] = false;
		}
		break;
	case '2':
		if (keydown[CS_KEY_PRESS_2] == true) {
			server_manager->SendPacket(CS_KEY_RELEASE_2);
			keydown[CS_KEY_PRESS_2] = false;
		}
		break;
	case 'W':
	case 'w':
		if (keydown[CS_KEY_PRESS_UP] == true) {
			server_manager->SendPacket(CS_KEY_RELEASE_UP);
			keydown[CS_KEY_PRESS_UP] = false;
		}
		break;
	case 'a':
	case 'A':
		if (keydown[CS_KEY_PRESS_LEFT] == true) {
			server_manager->SendPacket(CS_KEY_RELEASE_LEFT);
			keydown[CS_KEY_PRESS_LEFT] = false;
		}
		break;
	case 's':
	case 'S':
		if (keydown[CS_KEY_PRESS_DOWN] == true) {
			server_manager->SendPacket(CS_KEY_RELEASE_DOWN);
			keydown[CS_KEY_PRESS_DOWN] = false;
		}
		break;
	case 'D':
	case 'd':
		if (keydown[CS_KEY_PRESS_RIGHT] == true) {
			server_manager->SendPacket(CS_KEY_RELEASE_RIGHT);
			keydown[CS_KEY_PRESS_RIGHT] = false;
		}
		break;
	}

}
void SpecialKeyInput(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_SHIFT_L:
		if (keydown[CS_KEY_PRESS_SHIFT] == false) {
			server_manager->SendPacket(CS_KEY_PRESS_SHIFT);
			keydown[CS_KEY_PRESS_SHIFT] = true;
		}
		break;
	case GLUT_KEY_F5: 
		if (keydown[CS_PLAYER_READY] == false) {
			server_manager->SendPacket(CS_PLAYER_READY);
			keydown[CS_PLAYER_READY] = true;
		}
		else if (keydown[CS_PLAYER_READY] == true) {
			server_manager->SendPacket(CS_PLAYER_READY_CANCLE);
			keydown[CS_PLAYER_READY] = false;
		}
		break;

	}
}

void SpecialkeyRelease(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_SHIFT_L:
		if (keydown[CS_KEY_PRESS_SHIFT] == true) {
			server_manager->SendPacket(CS_KEY_RELEASE_SHIFT);
			keydown[CS_KEY_PRESS_SHIFT] = false;
		}
		break;
	case GLUT_KEY_F5:
		break;
	}
}