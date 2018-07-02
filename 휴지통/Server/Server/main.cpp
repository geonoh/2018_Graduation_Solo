#include "stdafx.h"
#include "SFW.h"
#define NUM_OF_THREADS 3

// Function Declare
void InitInstance();
void AcceptPlayer();
void WorkerThread();

// Global Var
SFW sfw;	//Server Framework

int main() {
	InitInstance();
	thread acpt_th{ AcceptPlayer };
	vector<thread*> wrk_th;
	for (int i = 0; i < NUM_OF_THREADS; ++i) {
		wrk_th.push_back(new thread{ WorkerThread });
	}


	for (int i = 0; i < NUM_OF_THREADS; ++i) {
		wrk_th[i]->join();
	}
	acpt_th.join();
	system("pause");
}

void InitInstance() {
	sfw.Init();
}
void AcceptPlayer() {
	while (true)
		sfw.AcceptPlayer();
}
void WorkerThread() {
	sfw.WorkerThread();
}