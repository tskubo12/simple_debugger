// simple_debugger.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "targetver.h"
#include<Windows.h>
#include<iostream>


int _tmain(int argc, _TCHAR* argv[])
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	int stopper;


	if (argc < 2) {
		_ftprintf(stderr, _T("C:\\>%s <sample.exe>\n"), argv[0]);
		std::cin >> stopper;
		return 1;
	}

	//構造体を全てゼロで埋める
	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(STARTUPINFO); //cb 構造体のサイズを指定する


	//第二引数の文字列をコマンドとして実行、今回の場合はコマンドライン引数で指定したファイルを実行、失敗すると0が返る
	//siにはプロセス立ち上げ時の設定が記述されている、piには新しいプロセスに関する情報が格納される
	BOOL r = CreateProcess(
		NULL, argv[1], NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED | DEBUG_PROCESS,
		NULL, NULL, &si, &pi);
	if (!r) {
		_ftprintf(stderr, _T("%s can't execute"), argv[1]);
		std::cin >> stopper;
		return -1;
	}

	//サスペンドカウンタをデクリメントし生成したプロセスを実行
	//サスペンド状態で対象のプロセスが起動されているためこのとき動作が開始する
	ResumeThread(pi.hThread);

	//無限ループ
	while (1) {
		DEBUG_EVENT de;

		//デバッグ中のプロセスへのハンドルを受け取り、失敗したときゼロが返ってくる
		//deに補足した情報を格納するまで無制限に待つ
		if (!WaitForDebugEvent(&de, INFINITE))
			break;

		//関数はすべての例外処理を停止してスレッドを続行する
		DWORD dwContinueStatus = DBG_CONTINUE;

		//上記でde内に格納されたデバッグイベントの種類を表示する
		switch (de.dwDebugEventCode) {
		case CREATE_PROCESS_DEBUG_EVENT:
			printf("CREATE_PROCESS_DEBUG_EVENT\n");
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			printf("CREATE_THREAD_DEBUG_EVENT\n");
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			printf("EXIT_THREAD_DEBUG_EVENT");
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			printf("EXIT_PROCESS_DEBUG_EVENT");
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			printf("OUTPUT_DEBUG_STARING_EVENT\n");
			break;
		case RIP_EVENT:
			printf("RIP_EVENT\n");
			break;
		case LOAD_DLL_DEBUG_EVENT:
			printf("LOAD_DLL_DEBUG_EVENT\n");
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			printf("UNLOAD_DLL_DEBUG_EVENT\n");
			break;
		case EXCEPTION_DEBUG_EVENT:
			DWORD r = de.u.Exception.ExceptionRecord.ExceptionCode;
			if (r != EXCEPTION_BREAKPOINT)
				dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
			printf("EXCEPTION_DEBUG_EVENT\n");
			break;

		}
		if (de.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
			break;
		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return 0;
}

