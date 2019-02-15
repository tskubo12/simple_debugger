// simple_debugger.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "targetver.h"
#include<Windows.h>


int main(int argc, _TCHAR* argv[])
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	if (argc < 2) {
		fprintf(stderr, "C:\\>%s <sample.exe>\n", argv[0]);
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
	if (!r) return -1;

	//サスペンドカウンタをデクリメントし生成したプロセスを実行
	ResumeThread(pi.hThread);

	while (1) {
		DEBUG_EVENT de;

		//デバッグ中のプロセスへのハンドルを受け取り、失敗したときゼロが返ってくる
		if (!WaitForDebugEvent(&de, INFINITE))
			break;

		//関数はすべての例外処理を停止してスレッドを続行する
		DWORD dwContinueStatus = DBG_CONTINUE;
	}
    return 0;
}

