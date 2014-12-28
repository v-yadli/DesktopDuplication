// DesktopDuplicationClient.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "DesktopDuplicationClient.h"
#define DDS_CLIENT
#include "CommonTypes.h"

#define MAX_LOADSTRING 100
#pragma comment(lib, "Msimg32.lib")

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

char* m_DDSBuffer;
INT m_ServerMode;
HANDLE m_MappedDDSBufferHandle;

TCHAR szName[]=TEXT("Global\\DesktopDDSBuffer");
#define DDS_BUFFER_SIZE (32*1024*1024)
#define MOUSE_PTR_INFO_OFFSET (16*1024*1024)

HWND windowHandle       = NULL;
HDC windowDC            = NULL;
HDC bitmapDC            = NULL;
HBITMAP bitmap          = NULL;
HPEN pen                = NULL;

HBITMAP mouseBitmap     = NULL;
HDC mouseDC             = NULL;
BLENDFUNCTION blendFunc;


BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup 
        &luid))        // receives LUID of privilege
    {
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES) NULL,
        (PDWORD) NULL))
    {
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
        return FALSE;
    }

    return TRUE;
}


void MapDDSBuffer()
{

    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        exit(-1);
    }
    if (!SetPrivilege(hToken, SE_CREATE_GLOBAL_NAME, TRUE))
    {
        exit(-1);
    }
    CloseHandle(hToken);


    m_MappedDDSBufferHandle = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        szName);
    if (m_MappedDDSBufferHandle == NULL)
    {
        exit(-1);
    }
    m_DDSBuffer = (char*) MapViewOfFile(m_MappedDDSBufferHandle, // handle to map object
                                        FILE_MAP_ALL_ACCESS,  // read/write permission
                                        0,
                                        0,
                                        DDS_BUFFER_SIZE);
}


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
VOID                DrawDDSBuffer();

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_ LPTSTR    lpCmdLine,
                       _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MapDDSBuffer();

    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_DESKTOPDUPLICATIONCLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DESKTOPDUPLICATIONCLIENT));

    // Main message loop:
    //while (GetMessage(&msg, NULL, 0, 0))
    //{
    //    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    //    {
    //        TranslateMessage(&msg);
    //        DispatchMessage(&msg);
    //    }
    //}

    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                PostQuitMessage(0);
                break;
            }
            TranslateMessage(&msg);	// translates 'character' keyboard messages
            DispatchMessage(&msg);	// send off to WndProc for processing
        }
        else
        {
            // could put more update code in here
            // before drawing.
            DrawDDSBuffer();
            Sleep(15);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DESKTOPDUPLICATIONCLIENT));
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH) (COLOR_WINDOW + 1);
    //wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_DESKTOPDUPLICATIONCLIENT);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance; // Store instance handle in our global variable

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
    lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    SetWindowLong(hWnd, GWL_STYLE, lStyle);

    MoveWindow(hWnd, 0, 0, 768, 1366, TRUE);
    windowHandle = hWnd;
    windowDC = GetDC(windowHandle);

    return TRUE;
}

void InitBitmap()
{
    bitmap = CreateCompatibleBitmap(windowDC, 768, 1366);
    bitmapDC = CreateCompatibleDC(windowDC);
    SelectObject(bitmapDC, bitmap);
    pen = CreatePen(PS_DASHDOTDOT, 2, NULL);

    mouseDC = CreateCompatibleDC(windowDC);
    mouseBitmap = CreateCompatibleBitmap(windowDC, 32, 32);

    SelectObject(mouseDC, mouseBitmap);

    blendFunc.BlendOp = AC_SRC_OVER;
    blendFunc.BlendFlags = 0;
    blendFunc.SourceConstantAlpha = 255;
    blendFunc.AlphaFormat = AC_SRC_ALPHA;
}

void DrawDDSBuffer()
{
    if (bitmap == NULL)
        InitBitmap();

    if (!SetBitmapBits(bitmap, 1366 * 768 * 4, m_DDSBuffer))
        throw "BitmapBits";

    PTR_INFO *ptr_info = (PTR_INFO*) (m_DDSBuffer + MOUSE_PTR_INFO_OFFSET);
    if (ptr_info->Visible)
    {
        //SelectObject(hdc, pen);
        //Ellipse(hdc, ptr_info->Position.x, ptr_info->Position.y, ptr_info->Position.x + 5, ptr_info->Position.y + 5);
        //TransparentBlt(hdc, ptr_info->Position.x, ptr_info->Position.y, 32, 32, mouseDC, 0, 0, 32, 32, 0x00000000);
        CURSORINFO ci;
        ci.cbSize = sizeof(ci);
        ci.flags = CURSOR_SHOWING;
        GetCursorInfo(&ci);
        DrawIconEx(mouseDC, 0, 0, ci.hCursor, 0, 0, 0, NULL, DI_NORMAL);

        AlphaBlend(bitmapDC, ptr_info->Position.x, ptr_info->Position.y, 32, 32, mouseDC, 0, 0, 32, 32, blendFunc);
    }

    //Commit to front buffer
    BitBlt(windowDC, 0, 0, 768, 1366, bitmapDC, 0, 0, SRCCOPY);

}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
    case WM_LBUTTONDBLCLK:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

