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

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
VOID                DrawDDSBuffer();

void MapDDSBuffer()
{
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

    int loop_tick = 0;

    // Main loop
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
            DrawDDSBuffer();
            Sleep(20);
            if (++loop_tick >= 100)
            {
                int x = rand() % 500;
                int y = rand() % 500;

                SetCursorPos(x, y);

                loop_tick = 0;
            }
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

int checksum = 0;
typedef int SCREEN_BUFFER_LINE[768];
typedef uint8_t POINT_VEC[4];
typedef float POINT_VEC_DEST[4];

SCREEN_BUFFER_LINE convolution_result[1366];

inline void VecAdd(POINT_VEC_DEST& v1, POINT_VEC& v2, float mul)
{
    v1[0] += v2[0] * mul;
    v1[1] += v2[1] * mul;
    v1[2] += v2[2] * mul;
    //v1[3] += v2[3] * mul;
}

inline void VecSeal(POINT_VEC_DEST& v)
{
    if (v[0] < 0)
        v[0] = 0;
    if (v[0] > 255)
        v[0] = 255;
    if (v[1] < 0)
        v[1] = 0;
    if (v[1] > 255)
        v[1] = 255;
    if (v[2] < 0)
        v[2] = 0;
    if (v[2] > 255)
        v[2] = 255;
    //if (v[3] < 0)
    //    v[3] = 0;
    //if (v[3] > 255)
    //    v[3] = 255;
}

inline int DoConvolution(SCREEN_BUFFER_LINE buffer[], int x, int y)
{
    //if (x == 0 || x == 767 || y == 0 || y == 1365)
    //    return buffer[y][x];

    POINT_VEC *current_point = (POINT_VEC*) &(buffer[y][x]);
    POINT_VEC *p1 = (POINT_VEC*) &(buffer[y - 1][x]);
    POINT_VEC *p2 = (POINT_VEC*) &(buffer[y + 1][x]);
    POINT_VEC *p3 = (POINT_VEC*) &(buffer[y][x - 1]);
    POINT_VEC *p4 = (POINT_VEC*) &(buffer[y][x + 1]);

    POINT_VEC_DEST result ={ .0f, .0f, .0f, .0f };

    VecAdd(result, *current_point, 5);
    VecAdd(result, *p1, -1);
    VecAdd(result, *p2, -1);
    VecAdd(result, *p3, -1);
    VecAdd(result, *p4, -1);

    VecSeal(result);

    int ret = (short) result[0] | ((short) result[1] << 8) | ((short) result[2] << 16) | ((short) result[3] << 24);
    return ret;
}

#define SCREEN_SEGMENT_SIZE 32

void DrawDDSBuffer()
{
    if (bitmap == NULL)
        InitBitmap();

    int current_checksum = 0;
    SCREEN_BUFFER_LINE *buffer_ptr = (SCREEN_BUFFER_LINE*) m_DDSBuffer;

    for (int y = 0; y < 1366; ++y)
        for (int x = 0; x < 768; ++x)
            current_checksum += buffer_ptr[y][x];

    if (current_checksum != checksum)
    {
        checksum = current_checksum;

        for (int y = 1; y < 1365; ++y)
        {
            for (int x = 1; x < 767; ++x)
            {
                convolution_result[y][x] = DoConvolution(buffer_ptr, x, y);
            }

        }

    }

    if (!SetBitmapBits(bitmap, 1366 * 768 * 4, convolution_result))
        throw "BitmapBits";

    PTR_INFO *ptr_info = (PTR_INFO*) (m_DDSBuffer + MOUSE_PTR_INFO_OFFSET);
    if (ptr_info->Visible)
    {
        LONG ptr_x = ptr_info->Position.x;
        LONG ptr_y = ptr_info->Position.y;

        ptr_x = ptr_x * 768 / 480;
        ptr_y = ptr_y * 1366 / 848;

        POINT cursor[3];
        cursor[0].x = ptr_x;
        cursor[0].y = ptr_y;
        cursor[1].x = ptr_x + 31;
        cursor[1].y = ptr_y + 31;
        cursor[2].x = ptr_x;
        cursor[2].y = ptr_y + 31;

        //Polygon(bitmapDC, cursor, 3);

        Polygon(bitmapDC, cursor, 3);
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

