// simple_debugger.cpp : アプリケーションのエントリ ポイントを定義します。
//
/*例外が発生したら発生した場所とその時のレジスタの値を表示する*/
#include "stdafx.h"
#include "targetver.h"
#include<Windows.h>
#include<iostream>
#include "udis86.h"

#pragma comment(lib, "libudis86.lib")

//ibo_funcが未解決シンボルであるためそれを解決する
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}

int disas(unsigned char *buff, char *out, int size) {
	ud_t ud_obj;
	ud_init(&ud_obj);

	ud_set_input_buffer(&ud_obj, buff, 32);
	ud_set_mode(&ud_obj, 32);
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);

	if (ud_disassemble(&ud_obj)) {
		sprintf_s(out, size, "%14s, %s",
			ud_insn_hex(&ud_obj), ud_insn_asm(&ud_obj));
	}
	else {
		return -1;
	}

	return (int)ud_insn_len(&ud_obj);

}

int exception_debug_event(DEBUG_EVENT *pde) {
	DWORD dwReadBytes;

	HANDLE ph = OpenProcess(
		PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION,
		FALSE, pde->dwProcessId);
	if (!ph)
		return -1;

	HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, pde->dwThreadId);
	if (!th)
		return -1;

	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_ALL;
	GetThreadContext(th, &ctx);

	char asm_string[256];
	unsigned char asm_code[32];

	ReadProcessMemory(ph, (VOID *)ctx.Eip, asm_code, 32, &dwReadBytes);
	if (disas(asm_code, asm_string, sizeof(asm_string)) == -1)
		asm_string[0] = '\0';


	printf("EXception: %08x (PID:%d, TID:%d)\n",
		pde->u.Exception.ExceptionRecord.ExceptionAddress,
		pde->dwProcessId, pde->dwThreadId);
	printf(" %08x: %s\n", ctx.Eip, asm_string);
	printf("   Reg: EAX=%08x ECX=%08x EDX=%08x EBX=%08x\n",
		ctx.Eax, ctx.Ecx, ctx.Edx, ctx.Ebx);
	printf("        ESI=%08x EDI=%08x ESP=%08x EBP=%08x\n",
		ctx.Esi, ctx.Edi, ctx.Esp, ctx.Ebp);

	SetThreadContext(th, &ctx);
	CloseHandle(th);
	CloseHandle(ph);
	return 0;
}


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

	int process_counter = 0;

	//無限ループ
	do
	{
		DEBUG_EVENT de;
		if (!WaitForDebugEvent(&de, INFINITE))
			break;

		DWORD dwContinueStatus = DBG_CONTINUE;

		switch (de.dwDebugEventCode) {
		case CREATE_PROCESS_DEBUG_EVENT:
			process_counter++;
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			process_counter--;
			break;
		case EXCEPTION_DEBUG_EVENT:
			if (de.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT) {
				dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
			}
			exception_debug_event(&de);
			break;
		}
		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);

	} while (process_counter > 0);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);


	std::cin >> stopper;

	return 0;
	//while(1){
	//	DEBUG_EVENT de;

	//	//デバッグ中のプロセスへのハンドルを受け取り、失敗したときゼロが返ってくる
	//	//deに補足した情報を格納するまで無制限に待つ
	//	if (!WaitForDebugEvent(&de, INFINITE))
	//		break;

	//	//関数はすべての例外処理を停止してスレッドを続行する
	//	DWORD dwContinueStatus = DBG_CONTINUE;

	//	//上記でde内に格納されたデバッグイベントの種類を表示する
	//	switch (de.dwDebugEventCode) {
	//	case CREATE_PROCESS_DEBUG_EVENT:
	//		printf("CREATE_PROCESS_DEBUG_EVENT\n");
	//		break;
	//	case CREATE_THREAD_DEBUG_EVENT:
	//		printf("CREATE_THREAD_DEBUG_EVENT\n");
	//		break;
	//	case EXIT_THREAD_DEBUG_EVENT:
	//		printf("EXIT_THREAD_DEBUG_EVENT");
	//		break;
	//	case EXIT_PROCESS_DEBUG_EVENT:
	//		printf("EXIT_PROCESS_DEBUG_EVENT");
	//		break;
	//	case OUTPUT_DEBUG_STRING_EVENT:
	//		printf("OUTPUT_DEBUG_STARING_EVENT\n");
	//		break;
	//	case RIP_EVENT:
	//		printf("RIP_EVENT\n");
	//		break;
	//	case LOAD_DLL_DEBUG_EVENT:
	//		printf("LOAD_DLL_DEBUG_EVENT\n");
	//		break;
	//	case UNLOAD_DLL_DEBUG_EVENT:
	//		printf("UNLOAD_DLL_DEBUG_EVENT\n");
	//		break;
	//	case EXCEPTION_DEBUG_EVENT:
	//		DWORD r = de.u.Exception.ExceptionRecord.ExceptionCode;
	//		if (r != EXCEPTION_BREAKPOINT)
	//			dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
	//		printf("EXCEPTION_DEBUG_EVENT\n");
	//		break;

	//	}
	//	if (de.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
	//		break;
	//	ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);
	//}
}

