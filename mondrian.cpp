// mondrian.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "automata.h"

void CheckError() {
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	MessageBox(NULL, (LPCTSTR) lpMsgBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
}

void InitGL(HWND hWnd, HDC& hDC, HGLRC& hRC) {

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof pfd);
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;

	hDC = GetDC(hWnd);

	int i = ChoosePixelFormat(hDC, &pfd);
	SetPixelFormat(hDC, i, &pfd);

	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);
}

void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC) {
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hWnd, hDC);
}

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static HDC hDC;
	static HGLRC hRC;

	const int TIMER = 1;

	switch(message) {
		case WM_CREATE:
		{
#ifdef _DEBUG
			SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 800, 600, 0);
#endif

			RECT rect;
			GetClientRect(hWnd, &rect);

			InitGL(hWnd, hDC, hRC);
			SetupAutomata(rect.right, rect.bottom);

			SetTimer(hWnd, TIMER, 10, NULL);
			return 0;
		}

		case WM_DESTROY:
		{
			KillTimer(hWnd, TIMER);

			CloseGL(hWnd, hDC, hRC);
			return 0;
		}

		case WM_TIMER:
		{
			TickAutomata();
			SwapBuffers(hDC);

			return 0;
		}

		case WM_KEYDOWN:
		{
			if (wParam == VK_RETURN) {
				ResetAutomata();

				return 0;
			}
		}

		default:
			return DefScreenSaverProc(hWnd, message, wParam, lParam);
	}
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	return false;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst) {
	return true;
}