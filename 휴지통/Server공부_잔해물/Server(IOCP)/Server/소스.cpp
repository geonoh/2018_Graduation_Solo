#include "NetworkDeclare.h"

#define SERVERPORT 9000

// 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	OVERLAPPED overlapped;
	SOCKET sock;
	char buf[sizeof(CtsPacket)];
	int recvbytes;
	int sendbytes;
	WSABUF wsabuf;
};

// 비동기 입출력 처리 함수
DWORD WINAPI WorkerThread(LPVOID arg);

int main(int argc, char *argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;


	// 입출력 완료포트 생성
	// (여기서 새로운 입출력 포트를 만드므로, 효력 없는 핸들값을 넣어준다.
	// 그리고 두 번째 인자로 NULL을 넣어서 새로운 입출력 포트임을 명시한다.
	// 
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) return 1;


	// CPU 개수 확인 
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	printf("Processor : %d\n", (int)si.dwNumberOfProcessors);

	// Processor * 2만큼 작업자 스레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; ++i) {
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);
	}
	printf("스레드 생성 완료 \n");
	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit((char*)"socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit((char*)"listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes, flags;


	while (1) {
		printf("수신 대기중\n");


		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display((char*)"accept()"); 
			break;
		}
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// listen 소켓으로부터 받아온 클라이언트 소켓과 
		// 입출력 완료포트를 연결해준다.
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);



		// -----------------------------------------------------------------------
		// 이 부분 있어야 되는지 의문
		// 이 부분 무조건 있어야한다.
		// 왜냐하면 일단 WSARecv를 클라이언트 하나 생성할 때마다 
		// 걸어주고 틈틈히 GetCompletionStatus로 확인

		// 소켓 정보 구조체 할당
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == NULL)break;
		CtsPacket client_buffer;
		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->recvbytes = ptr->sendbytes = 0;
		ptr->wsabuf.buf = ptr->buf;

		// 주의
		// Client에서 Server로 보낼 때 의 패킷과
		// Server에서 Client로 보내는 패킷의 사이즈 다름.
		// buffer 2개 사용해야 할 필요 있음.
		ptr->wsabuf.len = sizeof(CtsPacket);

		flags = 0;
		// 비동기 입출력 시작
		retval = WSARecv(client_sock, &ptr->wsabuf, 1, &recvbytes,
			&flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				err_display((char*)"WSARecv()");
			}
			continue;
		}

		//// 만약 혼란을 틈타서 접속하는게 있으면 이게 출력 될 것.
		memcpy(&client_buffer, ptr->buf, sizeof(ptr->buf));
		printf("[TCP/%s:%d] %x\n", inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port), client_buffer.keyboard_click);
		//// -----------------------------------------------------------------------

	}

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 비동기 입출력 처리 함수
DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;
	CtsPacket cts_buffer;
	ZeroMemory(&cts_buffer, sizeof(cts_buffer));

	while (true) {
		// 비동기 입출력 완료 기다리기 
		DWORD cbTransferred;  // CbTransferred는 비동기 입출력 작업으로 전송된 바이트 수.
		SOCKET client_sock;
		SOCKETINFO* ptr;

		//retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
		//	(LPDWORD)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);
		
		printf("쓰레드 대기중 \n");
		// 위에 안되서 PULONG_PTR을 사용함
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred,		
			(PULONG_PTR)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);
		printf("큐에 완료된 포트 발견\n");

		// 클라이언트 정보 불러오기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);


		// 비동기 입출력 결과 확인
		if (retval == 0 || cbTransferred == 0) {
			if (retval == 0) {
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &ptr->overlapped,
					&temp1, FALSE, &temp2);
				err_display((char*)"WSAGetOverlappedResult()");
			}
			closesocket(ptr->sock);
			printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			delete ptr;
			continue;
		}


		// 데이터 전송량 갱신
		// but 지금은 클라이언트에서 오는 데이터
		// 수신만 하므로 무조건적으로 recv만 
		if (ptr->recvbytes == 0) {
			ptr->recvbytes = cbTransferred;
			ptr->sendbytes == 0;
			// 받은 데이터 출력
			memcpy(&cts_buffer, ptr->buf, sizeof(ptr->buf));
			printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
				ntohs(clientaddr.sin_port), cts_buffer.keyboard_click);
		}
		else {	// 받은 데이터가 있는 경우
			ptr->sendbytes += cbTransferred;
		}



		////----------------------------------------------------------------------------
		//// 애초에 이 작업이 필요 없는거같다.


		//// 데이터 보내기
		//if (ptr->recvbytes > ptr->sendbytes) {
		//	ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		//	ptr->wsabuf.buf = ptr->buf + ptr->sendbytes;
		//	// 여기서 sendbyte를 왜 더하는거지?
		//	ptr->wsabuf.len = ptr->recvbytes - ptr->sendbytes;

		//	DWORD sendbytes;
		//	retval = WSASend(ptr->sock, &ptr->wsabuf, 1,
		//		&sendbytes, 0, &ptr->overlapped, NULL);
		//	if (retval == SOCKET_ERROR) {
		//		if (WSAGetLastError() != WSA_IO_PENDING) {
		//			err_display((char*)"WSASend()");
		//		}
		//		continue;
		//	}
		//}

		//// 데이터 받기
		//// 꼭 주거니 받거니가 필요한 상황이면
		//// 이런식으로 프로그램 해야하지만
		//// 내가 만들 서버에서는 이렇게 까지는 할 필요가
		//// 없다. 
		//else {
		//	ptr->recvbytes = 0;

		//	ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		//	ptr->wsabuf.buf = ptr->buf;
		//	ptr->wsabuf.len = sizeof(CtsPacket);

		//	DWORD recvbytes;
		//	DWORD flags = 0;
		//	retval = WSARecv(ptr->sock, &ptr->wsabuf, 1,
		//		&recvbytes, &flags, &ptr->overlapped, NULL);
		//	if (retval == SOCKET_ERROR) {
		//		if (WSAGetLastError() != WSA_IO_PENDING) {
		//			err_display((char*)"WSARecv()");
		//		}
		//		continue;
		//	}
		//}


		////----------------------------------------------------------------------------




	}




	return retval; 
}

