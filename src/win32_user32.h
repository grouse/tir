#ifndef WIN32_USER32_H
#define WIN32_USER32_H

#include "win32_lite.h"

#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEA

#define IDC_SIZENWSE MAKEINTRESOURCE(32642)
#define IDC_ARROW MAKEINTRESOURCE(32512)

#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CXVIRTUALSCREEN 78
#define SM_CYVIRTUALSCREEN 79

#define COLOR_WINDOW 5

#define TME_LEAVE 0x00000002

#define RIDEV_REMOVE 0x00000001
#define RIDEV_EXCLUDE 0x00000010
#define RIDEV_PAGEONLY 0x00000020
#define RIDEV_NOLEGACY 0x00000030
#define RIDEV_INPUTSINK 0x00000100
#define RIDEV_CAPTUREMOUSE 0x00000200
#define RIDEV_NOHOTKEYS 0x00000200
#define RIDEV_APPKEYS 0x00000400
#define RIDEV_EXINPUTSINK 0x00001000
#define RIDEV_DEVNOTIFY 0x00002000

#define RIM_INPUT 0
#define RIM_INPUTSINK 1

#define RIM_TYPEMOUSE 0
#define RIM_TYPEKEYBOARD 1
#define RIM_TYPEHID 2

#define RI_MOUSE_MOVE_RELATIVE 0x00
#define RI_MOUSE_MOVE_ABSOLUTE 0x01
#define RI_MOUSE_VIRTUAL_DESKTOP 0x02
#define RI_MOUSE_ATTRIBUTES_CHANGED 0x04
#define RI_MOUSE_MOVE_NOCOALESCE 0x08

#define RI_MOUSE_BUTTON_1_DOWN 0x0001
#define RI_MOUSE_BUTTON_1_UP 0x0002
#define RI_MOUSE_BUTTON_2_DOWN 0x0004
#define RI_MOUSE_BUTTON_2_UP 0x0008
#define RI_MOUSE_BUTTON_3_DOWN 0x0010
#define RI_MOUSE_BUTTON_3_UP 0x0020
#define RI_MOUSE_BUTTON_4_DOWN 0x0040
#define RI_MOUSE_BUTTON_4_UP 0x0080
#define RI_MOUSE_BUTTON_5_DOWN 0x0100
#define RI_MOUSE_BUTTON_5_UP 0x0200
#define RI_MOUSE_WHEEL 0x0400
#define RI_MOUSE_HWHEEL 0x0800

#define RID_HEADER 0x10000005
#define RID_INPUT 0x10000003

#define WM_INPUT                        0x00FF
#define WM_SHOWWINDOW                   0x0018
#define WM_TIMER                        0x0113
#define WM_QUIT                         0x0012
#define WM_CLOSE                        0x0010
#define WM_KEYDOWN                      0x0100
#define WM_KEYUP                        0x0101
#define WM_SYSKEYDOWN                   0x0104
#define WM_SYSKEYUP                     0x0105
#define WM_MOUSEMOVE                    0x0200
#define WM_MOUSEWHEEL                   0x020A
#define WM_MOUSELEAVE                   0x02A3
#define WM_LBUTTONDOWN                  0x0201
#define WM_LBUTTONUP                    0x0202
#define WM_MBUTTONDOWN                  0x0207
#define WM_MBUTTONUP                    0x0208
#define WM_RBUTTONDOWN                  0x0204
#define WM_RBUTTONUP                    0x0205
#define WM_XBUTTONDOWN                  0x020B
#define WM_XBUTTONUP                    0x020C
#define WM_PAINT                        0x000F
#define WM_CHAR                         0x0102
#define WM_COMMAND                      0x0111
#define WM_SIZE                         0x0005
#define WM_SIZING                       0x0214
#define WM_ENTERSIZEMOVE                0x0231
#define WM_EXITSIZEMOVE                 0x0232
#define WM_SETCURSOR                    0x0020
#define WM_NCHITTEST                    0x0084

#define HTCLIENT                        1

#define MK_CONTROL  0x0008
#define MK_LBUTTON  0x0001
#define MK_MBUTTON  0x0010
#define MK_RBUTTON  0x0002
#define MK_SHIFT    0x0004
#define MK_XBUTTON1 0x0020
#define MK_XBUTTON2 0x0040

#define XBUTTON1    0x0001
#define XBUTTON2    0x0002

typedef struct tagPAINTSTRUCT {
    HDC  hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagTRACKMOUSEEVENT {
  DWORD cbSize;
  DWORD dwFlags;
  HWND  hwndTrack;
  DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

typedef struct tagRAWINPUTDEVICE {
  USHORT usUsagePage;
  USHORT usUsage;
  DWORD  dwFlags;
  HWND   hwndTarget;
} RAWINPUTDEVICE, *PRAWINPUTDEVICE, *LPRAWINPUTDEVICE;
typedef CONST RAWINPUTDEVICE* PCRAWINPUTDEVICE;

typedef struct tagRAWINPUTHEADER {
  DWORD  dwType;
  DWORD  dwSize;
  HANDLE hDevice;
  WPARAM wParam;
} RAWINPUTHEADER, *PRAWINPUTHEADER, *LPRAWINPUTHEADER;

typedef struct tagRAWMOUSE {
  USHORT usFlags;
  union {
    ULONG ulButtons;
    struct {
      USHORT usButtonFlags;
      USHORT usButtonData;
    };
  };
  ULONG  ulRawButtons;
  LONG   lLastX;
  LONG   lLastY;
  ULONG  ulExtraInformation;
} RAWMOUSE, *PRAWMOUSE, *LPRAWMOUSE;

typedef struct tagRAWKEYBOARD {
  USHORT MakeCode;
  USHORT Flags;
  USHORT Reserved;
  USHORT VKey;
  UINT   Message;
  ULONG  ExtraInformation;
} RAWKEYBOARD, *PRAWKEYBOARD, *LPRAWKEYBOARD;

typedef struct tagRAWHID {
  DWORD dwSizeHid;
  DWORD dwCount;
  BYTE  bRawData[1];
} RAWHID, *PRAWHID, *LPRAWHID;

typedef struct tagRAWINPUT {
  RAWINPUTHEADER header;
  union {
    RAWMOUSE    mouse;
    RAWKEYBOARD keyboard;
    RAWHID      hid;
  } data;
} RAWINPUT, *PRAWINPUT, *LPRAWINPUT;
typedef HANDLE HRAWINPUT;

extern "C" {
    BOOL OpenClipboard(HWND hWndNewOwner);
    BOOL CloseClipboard();

    BOOL EmptyClipboard();

    HANDLE GetClipboardData(UINT uFormat);
    HANDLE SetClipboardData(UINT uFormat, HANDLE hMem);

    BOOL PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
    BOOL GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
    BOOL TranslateMessage(const MSG *lpMsg);
    LRESULT DispatchMessageA(const MSG *lpMsg);

    int FillRect(HDC hDC, const RECT *lprc, HBRUSH hbr);

    HDC BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint);
    BOOL EndPaint(HWND hWnd, const PAINTSTRUCT *lpPaint);

    ATOM RegisterClassA(const WNDCLASSA *lpWndClass);

    HDC GetDC(HWND hWnd);
    int ReleaseDC(HWND hWnd, HDC hDC);

    BOOL GetClientRect(HWND hWnd, LPRECT lpRect);
    BOOL ScreenToClient(HWND hWnd, LPPOINT lpPoint);
    int GetSystemMetrics(int nIndex);

    HWND CreateWindowExA(
        DWORD dwExStyle,
        LPCSTR lpClassName,
        LPCSTR lpWindowName,
        DWORD dwStyle,
        int X,
        int Y,
        int nWidth,
        int nHeight,
        HWND hWndParent,
        HMENU hMenu,
        HINSTANCE hInstance,
        LPVOID lpParam);

    BOOL DestroyWindow(HWND hWnd);

    LRESULT DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    BOOL SetWindowTextA(HWND hWnd, LPCSTR lpString);
    BOOL ShowWindow(HWND hWnd, int nCmdShow);
    HWND SetCapture(HWND hWnd);
    BOOL ReleaseCapture();

    BOOL TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);
    SHORT GetAsyncKeyState(int vKey);

    HCURSOR LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName);
    HCURSOR SetCursor(HCURSOR hCursor);
    BOOL GetCursorPos(LPPOINT lpPoint);
    int ShowCursor(BOOL bShow);

    BOOL RegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);
    UINT GetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);

}

#endif // WIN32_USER32_H
