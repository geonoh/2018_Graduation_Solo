#include "stdafx.h"
#include "ServerMgr.h"
#include "Renderer.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DoDisplay();
HINSTANCE g_hInst;
HWND hWndMain;
LPCTSTR lpszClass = TEXT("TheBoat_Client");
HDC hdc;
HGLRC hrc;

//Init Things
void Initinstance();
void InitSocket();

// Renderer
Renderer *g_Renderer = nullptr;
float g_prevX = 0.f;
float g_prevY = 0.f;
float g_accX = 0.f;
float g_accY = 0.f;

float gTime = 0.f;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	int argc = 0;
	char** argv = 0;

	InitSocket();
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;


	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	if (GLEW_OK != glewInit())
	{
		printf("GLEW FAIL\n");
	}
	if (glewIsSupported("GL_VERSION_3_0"))
	{
		std::cout << " GLEW Version is 3.0\n ";
	}
	else
	{
		std::cout << "GLEW 3.0 not supported\n ";
	}
	Initinstance();

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return (int)Message.wParam;
}

void Initinstance() {

	g_Renderer = new Renderer(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!g_Renderer->IsInitialized())
	{
		std::cout << "Renderer could not be initialized.. \n";
	}

}

void DoDisplay()
{
	//glClear(GL_COLOR_BUFFER_BIT);

	//glBegin(GL_TRIANGLES);
	//glVertex2f(0.0, 0.5);
	//glVertex2f(-0.5, -0.5);
	//glVertex2f(0.5, -0.5);
	//glEnd();
	//glFinish();
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	//glClearDepth(1.f);

	//g_Renderer->SetRotation(g_accY, g_accX);

	////g_Renderer->FillAll(0, 0, 0, 0.4);

	//// Renderer Test
	////g_Renderer->DrawSTParticle(-1, 0, 1, 0, gTime);
	//float centers[] = { -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5 };
	////g_Renderer->Test(centers, gTime);
	////g_Renderer->Radar(centers, gTime);
	////g_Renderer->Bogang();
	////g_Renderer->TextureApp();
	////g_Renderer->TextureAnim();
	////g_Renderer->ProxyGeo();
	////g_Renderer->DrawParticle();
	//g_Renderer->DrawSkyBox();
	//g_Renderer->DrawHeightMap();
	//g_Renderer->Cube();
	////g_Renderer->Test1();


	//glutSwapBuffers();

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage) {
	case WM_KEYDOWN:
		std::cout << ("키 입력됨\n");
		break;
	case WM_CREATE:
		hWndMain = hWnd;
		PIXELFORMATDESCRIPTOR pfd;
		int nPixelFormat;

		hdc = GetDC(hWnd);
		memset(&pfd, 0, sizeof(pfd));

		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW |
			PFD_SUPPORT_OPENGL;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;

		nPixelFormat = ChoosePixelFormat(hdc, &pfd);
		SetPixelFormat(hdc, nPixelFormat, &pfd);

		hrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hrc);
		return 0;
	case WM_PAINT:
		DoDisplay();
		ValidateRect(hWnd, NULL);
		return 0;
	//case WM_SIZE:
	//	glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
	//	glMatrixMode(GL_PROJECTION);
	//	glLoadIdentity();

	//	glOrtho(-1, 1, -1, 1, 1, -1);

	//	glMatrixMode(GL_MODELVIEW);
	//	glLoadIdentity();
	//	return 0;
	case WM_DESTROY:
		//wglMakeCurrent(hdc, NULL);
		//wglDeleteContext(hrc);
		ReleaseDC(hWnd, hdc);
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void InitSocket() {

}

//
//#include "stdafx.h"
//#include "TheBoat_Client(OpenGL).h"
//
//#define MAX_LOADSTRING 100
//
//// 전역 변수:
//HINSTANCE hInst;                                // 현재 인스턴스입니다.
//WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
//WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
//
//												// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
//ATOM                MyRegisterClass(HINSTANCE hInstance);
//BOOL                InitInstance(HINSTANCE, int);
//LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
//INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
//
//int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
//	_In_opt_ HINSTANCE hPrevInstance,
//	_In_ LPWSTR    lpCmdLine,
//	_In_ int       nCmdShow)
//{
//	UNREFERENCED_PARAMETER(hPrevInstance);
//	UNREFERENCED_PARAMETER(lpCmdLine);
//
//	// TODO: 여기에 코드를 입력합니다.
//
//	// 전역 문자열을 초기화합니다.
//	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
//	LoadStringW(hInstance, IDC_THEBOATCLIENTOPENGL, szWindowClass, MAX_LOADSTRING);
//	MyRegisterClass(hInstance);
//
//	// 응용 프로그램 초기화를 수행합니다.
//	if (!InitInstance(hInstance, nCmdShow))
//	{
//		return FALSE;
//	}
//
//	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_THEBOATCLIENTOPENGL));
//
//	MSG msg;
//
//	// 기본 메시지 루프입니다.
//	while (GetMessage(&msg, nullptr, 0, 0))
//	{
//		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
//		{
//			TranslateMessage(&msg);
//			DispatchMessage(&msg);
//		}
//	}
//
//	return (int)msg.wParam;
//}
//
//
//
////
////  함수: MyRegisterClass()
////
////  목적: 창 클래스를 등록합니다.
////
//ATOM MyRegisterClass(HINSTANCE hInstance)
//{
//	WNDCLASSEXW wcex;
//
//	wcex.cbSize = sizeof(WNDCLASSEX);
//
//	wcex.style = CS_HREDRAW | CS_VREDRAW;
//	wcex.lpfnWndProc = WndProc;
//	wcex.cbClsExtra = 0;
//	wcex.cbWndExtra = 0;
//	wcex.hInstance = hInstance;
//	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_THEBOATCLIENTOPENGL));
//	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
//	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_THEBOATCLIENTOPENGL);
//	wcex.lpszClassName = szWindowClass;
//	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
//
//	return RegisterClassExW(&wcex);
//}
//
////
////   함수: InitInstance(HINSTANCE, int)
////
////   목적: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
////
////   설명:
////
////        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
////        주 프로그램 창을 만든 다음 표시합니다.
////
//BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
//{
//	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.
//
//	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
//		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
//
//	if (!hWnd)
//	{
//		return FALSE;
//	}
//
//	ShowWindow(hWnd, nCmdShow);
//	UpdateWindow(hWnd);
//
//	return TRUE;
//}
//
////
////  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
////
////  목적:  주 창의 메시지를 처리합니다.
////
////  WM_COMMAND  - 응용 프로그램 메뉴를 처리합니다.
////  WM_PAINT    - 주 창을 그립니다.
////  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
////
////
//LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	switch (message)
//	{
//	case WM_COMMAND:
//	{
//		int wmId = LOWORD(wParam);
//		// 메뉴 선택을 구문 분석합니다.
//		switch (wmId)
//		{
//		case IDM_ABOUT:
//			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
//			break;
//		case IDM_EXIT:
//			DestroyWindow(hWnd);
//			break;
//		default:
//			return DefWindowProc(hWnd, message, wParam, lParam);
//		}
//	}
//	break;
//	case WM_PAINT:
//	{
//		PAINTSTRUCT ps;
//		HDC hdc = BeginPaint(hWnd, &ps);
//		// TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다.
//		EndPaint(hWnd, &ps);
//	}
//	break;
//	case WM_DESTROY:
//		PostQuitMessage(0);
//		break;
//	default:
//		return DefWindowProc(hWnd, message, wParam, lParam);
//	}
//	return 0;
//}
//
//// 정보 대화 상자의 메시지 처리기입니다.
//INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	UNREFERENCED_PARAMETER(lParam);
//	switch (message)
//	{
//	case WM_INITDIALOG:
//		return (INT_PTR)TRUE;
//
//	case WM_COMMAND:
//		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
//		{
//			EndDialog(hDlg, LOWORD(wParam));
//			return (INT_PTR)TRUE;
//		}
//		break;
//	}
//	return (INT_PTR)FALSE;
//}
