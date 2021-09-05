#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

#define RuntimeAssert(x) if(!(x)){ __debugbreak();}
#define InvalidCodePath() RuntimeAssert(false)

#define RestrictModifier __restrict__

#define UnusedArgument(x) (void)x

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

#include "..\shimmer_api_generator.hpp"

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

void allocateDebugConsole()
{
	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS);
	freopen("CONOUT$", "w", stdout);
}

LRESULT CALLBACK K15_WNDPROC(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	uint8_t messageHandled = 0;

	switch (msg)
	{
	case WM_CREATE:
		break;

	case WM_CLOSE:
		PostQuitMessage(0);
		messageHandled = 1;
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		break;

	case WM_MOUSEMOVE:
		break;

	case WM_MOUSEWHEEL:
		break;
	}

	if (messageHandled == 0)
	{
		return DefWindowProc(hWnd, msg, wparam, lparam);
	}

	return 0;
}

HWND setupWindow(HINSTANCE p_Instance, int p_Width, int p_Height)
{
	WNDCLASS wndClass = {0};
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wndClass.hInstance = p_Instance;
	wndClass.lpszClassName = "K15_Win32Template";
	wndClass.lpfnWndProc = K15_WNDPROC;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);

	HWND hwnd = CreateWindowA("K15_Win32Template", "Win32 Template",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		p_Width, p_Height, 0, 0, p_Instance, 0);

	if (hwnd == INVALID_HANDLE_VALUE)
		MessageBox(0, "Error creating Window.\n", "Error!", 0);
	else
		ShowWindow(hwnd, SW_SHOW);
	return hwnd;
}

void setup()
{
	allocateDebugConsole();
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

	HWND hwnd = setupWindow(hInstance, 1024, 768);

	if (hwnd == INVALID_HANDLE_VALUE)
		return -1;

	setup();

	uint8_t loopRunning = 1;
	MSG msg;
	while (loopRunning)
	{
		while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				loopRunning = 0;
		}

		Sleep(10);
	}

	return 0;
}