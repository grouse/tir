#ifndef WIN32_LITE_H
#define WIN32_LITE_H

#include "platform.h"
#include "core.h"

#define CALLBACK __stdcall
#define WINAPI __stdcall
#define CONST const

#define DECLSPEC_ALLOCATOR __declspec(allocator)

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define TIMEOUT_INFINITE 0xFFFFFFFF
#define WIN32_MAX_PATH 260

#define GENERIC_READ                    0x80000000L
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

#define FILE_SHARE_READ                 0x00000001
#define FILE_SHARE_WRITE                0x00000002
#define FILE_SHARE_DELETE               0x00000004

#define ERROR_FILE_NOT_FOUND 0x2
#define ERROR_SHARING_VIOLATION 0x20
#define ERROR_ALREADY_EXISTS 0xB7
#define ERROR_INSUFFICIENT_BUFFER 0x7A

#define FILE_LIST_DIRECTORY 1

#define FILE_NOTIFY_CHANGE_LAST_WRITE 0x00000010
#define FILE_NOTIFY_CHANGE_FILE_NAME 0x00000001

#define FILE_ACTION_MODIFIED 0x00000003
#define FILE_ACTION_REMOVED 0x00000002
#define FILE_ACTION_ADDED 0x00000001

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define FILE_ATTRIBUTE_NORMAL           0x00000080
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000

#define STARTF_USESTDHANDLES 0x00000100

#define HANDLE_FLAG_INHERIT 0x00000001

#define CREATE_NO_WINDOW 0x08000000

#define GMEM_MOVEABLE 0x0002

#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_RESET 0x00080000
#define MEM_RESET_UNDO 0x1000000
#define MEM_LARGE_PAGES 0x20000000
#define MEM_PHYSICAL 0x00400000
#define MEM_TOP_DOWN 0x00100000
#define MEM_WRITE_WATCH 0x00200000

#define PAGE_EXECUTE 0x10
#define PAGE_EXECUTE_READ 0x20
#define PAGE_READWRITE 0x04


#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000

#define CS_OWNDC 0x0020

#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001

#define WS_OVERLAPPED 0x00000000L
#define WS_CAPTION 0x00C00000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_MINIMIZEBOX 0x00020000L
#define WS_MAXIMIZEBOX 0x00010000L

#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)

#define WS_EX_WINDOWEDGE 0x00000100L
#define WS_EX_CLIENTEDGE 0x00000200L
#define WS_EX_OVERLAPPEDWINDOW (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)

#define CW_USEDEFAULT ((int)0x80000000)

#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))

#define SW_SHOW 5

#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT 0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010
#define MK_XBUTTON1 0x0020
#define MK_XBUTTON2 0x0040


#define SIZE_MAXHIDE 4
#define SIZE_MAXIMIZED 2
#define SIZE_MAXSHOW 3
#define SIZE_MINIMIZED 1
#define SIZE_RESTORED 0

#define PFD_DRAW_TO_WINDOW          0x00000004
#define PFD_SUPPORT_OPENGL          0x00000020
#define PFD_DOUBLEBUFFER            0x00000001
#define PFD_TYPE_RGBA               0x0
#define PFD_MAIN_PLANE              0x0

/* Virtual Keys, Standard Set */
#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_CANCEL         0x03
#define VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */

#if(_WIN32_WINNT >= 0x0500)
#define VK_XBUTTON1       0x05    /* NOT contiguous with L & RBUTTON */
#define VK_XBUTTON2       0x06    /* NOT contiguous with L & RBUTTON */
#endif /* _WIN32_WINNT >= 0x0500 */

#define VK_BACK           0x08
#define VK_TAB            0x09
#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14
#define VK_KANA           0x15
#define VK_HANGEUL        0x15  /* old name - should be here for compatibility */
#define VK_HANGUL         0x15
#define VK_JUNJA          0x17
#define VK_FINAL          0x18
#define VK_HANJA          0x19
#define VK_KANJI          0x19
#define VK_ESCAPE         0x1B
#define VK_CONVERT        0x1C
#define VK_NONCONVERT     0x1D
#define VK_ACCEPT         0x1E
#define VK_MODECHANGE     0x1F
#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

/*
 * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x3A - 0x40 : unassigned
 * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */

#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D
#define VK_SLEEP          0x5F
#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87

#if(_WIN32_WINNT >= 0x0604)
#define VK_NAVIGATION_VIEW     0x88 // reserved
#define VK_NAVIGATION_MENU     0x89 // reserved
#define VK_NAVIGATION_UP       0x8A // reserved
#define VK_NAVIGATION_DOWN     0x8B // reserved
#define VK_NAVIGATION_LEFT     0x8C // reserved
#define VK_NAVIGATION_RIGHT    0x8D // reserved
#define VK_NAVIGATION_ACCEPT   0x8E // reserved
#define VK_NAVIGATION_CANCEL   0x8F // reserved
#endif /* _WIN32_WINNT >= 0x0604 */

#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91
#define VK_OEM_NEC_EQUAL  0x92   // '=' key on numpad
#define VK_OEM_FJ_JISHO   0x92   // 'Dictionary' key
#define VK_OEM_FJ_MASSHOU 0x93   // 'Unregister word' key
#define VK_OEM_FJ_TOUROKU 0x94   // 'Register word' key
#define VK_OEM_FJ_LOYA    0x95   // 'Left OYAYUBI' key
#define VK_OEM_FJ_ROYA    0x96   // 'Right OYAYUBI' key
#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5

#if(_WIN32_WINNT >= 0x0500)
#define VK_BROWSER_BACK        0xA6
#define VK_BROWSER_FORWARD     0xA7
#define VK_BROWSER_REFRESH     0xA8
#define VK_BROWSER_STOP        0xA9
#define VK_BROWSER_SEARCH      0xAA
#define VK_BROWSER_FAVORITES   0xAB
#define VK_BROWSER_HOME        0xAC

#define VK_VOLUME_MUTE         0xAD
#define VK_VOLUME_DOWN         0xAE
#define VK_VOLUME_UP           0xAF
#define VK_MEDIA_NEXT_TRACK    0xB0
#define VK_MEDIA_PREV_TRACK    0xB1
#define VK_MEDIA_STOP          0xB2
#define VK_MEDIA_PLAY_PAUSE    0xB3
#define VK_LAUNCH_MAIL         0xB4
#define VK_LAUNCH_MEDIA_SELECT 0xB5
#define VK_LAUNCH_APP1         0xB6
#define VK_LAUNCH_APP2         0xB7
#endif /* _WIN32_WINNT >= 0x0500 */

#define VK_OEM_1          0xBA   // ';:' for US
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define VK_OEM_2          0xBF   // '/?' for US
#define VK_OEM_3          0xC0   // '`~' for US

#if(_WIN32_WINNT >= 0x0604)
#define VK_GAMEPAD_A                         0xC3 // reserved
#define VK_GAMEPAD_B                         0xC4 // reserved
#define VK_GAMEPAD_X                         0xC5 // reserved
#define VK_GAMEPAD_Y                         0xC6 // reserved
#define VK_GAMEPAD_RIGHT_SHOULDER            0xC7 // reserved
#define VK_GAMEPAD_LEFT_SHOULDER             0xC8 // reserved
#define VK_GAMEPAD_LEFT_TRIGGER              0xC9 // reserved
#define VK_GAMEPAD_RIGHT_TRIGGER             0xCA // reserved
#define VK_GAMEPAD_DPAD_UP                   0xCB // reserved
#define VK_GAMEPAD_DPAD_DOWN                 0xCC // reserved
#define VK_GAMEPAD_DPAD_LEFT                 0xCD // reserved
#define VK_GAMEPAD_DPAD_RIGHT                0xCE // reserved
#define VK_GAMEPAD_MENU                      0xCF // reserved
#define VK_GAMEPAD_VIEW                      0xD0 // reserved
#define VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON    0xD1 // reserved
#define VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON   0xD2 // reserved
#define VK_GAMEPAD_LEFT_THUMBSTICK_UP        0xD3 // reserved
#define VK_GAMEPAD_LEFT_THUMBSTICK_DOWN      0xD4 // reserved
#define VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT     0xD5 // reserved
#define VK_GAMEPAD_LEFT_THUMBSTICK_LEFT      0xD6 // reserved
#define VK_GAMEPAD_RIGHT_THUMBSTICK_UP       0xD7 // reserved
#define VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN     0xD8 // reserved
#define VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT    0xD9 // reserved
#define VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT     0xDA // reserved
#endif /* _WIN32_WINNT >= 0x0604 */

#define VK_OEM_4          0xDB  //  '[{' for US
#define VK_OEM_5          0xDC  //  '\|' for US
#define VK_OEM_6          0xDD  //  ']}' for US
#define VK_OEM_7          0xDE  //  ''"' for US
#define VK_OEM_8          0xDF

#define VK_OEM_AX         0xE1  //  'AX' key on Japanese AX kbd
#define VK_OEM_102        0xE2  //  "<>" or "\|" on RT 102-key kbd.
#define VK_ICO_HELP       0xE3  //  Help key on ICO
#define VK_ICO_00         0xE4  //  00 key on ICO

#if(WINVER >= 0x0400)
#define VK_PROCESSKEY     0xE5
#endif /* WINVER >= 0x0400 */

#define VK_ICO_CLEAR      0xE6

#if(_WIN32_WINNT >= 0x0500)
#define VK_PACKET         0xE7
#endif /* _WIN32_WINNT >= 0x0500 */

#define VK_OEM_RESET      0xE9
#define VK_OEM_JUMP       0xEA
#define VK_OEM_PA1        0xEB
#define VK_OEM_PA2        0xEC
#define VK_OEM_PA3        0xED
#define VK_OEM_WSCTRL     0xEE
#define VK_OEM_CUSEL      0xEF
#define VK_OEM_ATTN       0xF0
#define VK_OEM_FINISH     0xF1
#define VK_OEM_COPY       0xF2
#define VK_OEM_AUTO       0xF3
#define VK_OEM_ENLW       0xF4
#define VK_OEM_BACKTAB    0xF5

#define VK_ATTN           0xF6
#define VK_CRSEL          0xF7
#define VK_EXSEL          0xF8
#define VK_EREOF          0xF9
#define VK_PLAY           0xFA
#define VK_ZOOM           0xFB
#define VK_NONAME         0xFC
#define VK_PA1            0xFD
#define VK_OEM_CLEAR      0xFE

#define CF_TEXT           1
#define CF_UNICODETEXT    13

#define NULL 0

typedef wchar_t WCHAR;
typedef char CHAR;
typedef CHAR *LPSTR;
typedef WCHAR *LPWSTR;
typedef WCHAR *PWSTR;
typedef CONST CHAR *LPCSTR;
typedef CONST WCHAR *LPCWSTR;
typedef unsigned char BYTE;
typedef BYTE* LPBYTE;

typedef int BOOL;

typedef void *PVOID;
typedef void *LPVOID;
typedef CONST void *LPCVOID;

typedef PVOID HANDLE;
typedef HANDLE* PHANDLE;
typedef HANDLE HICON;
typedef HANDLE HGLOBAL;
typedef HANDLE HINSTANCE;
typedef HANDLE HBRUSH;
typedef HANDLE HWND;
typedef HANDLE HMENU;
typedef HANDLE HDC;
typedef HANDLE HGLRC;
typedef HANDLE HMODULE;
typedef HICON HCURSOR;

typedef unsigned short WORD;
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef long long LONGLONG;
typedef float FLOAT;

typedef DWORD *LPDWORD;
typedef UINT *PUINT;

typedef WORD ATOM;
typedef __int64 LONG_PTR;
typedef unsigned __int64 ULONG_PTR;
typedef ULONG_PTR DWORD_PTR;
typedef signed __int64 INT_PTR;
typedef unsigned __int64 UINT_PTR;
typedef ULONG_PTR SIZE_T;

typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef LONG HRESULT;
typedef UINT_PTR WPARAM;

extern "C" {
    typedef LRESULT (CALLBACK* WNDPROC) (HWND, UINT, WPARAM, LPARAM);
    typedef INT_PTR (*PROC)();
    typedef INT_PTR (*FARPROC)();
    typedef INT_PTR (*NEARPROC)();
    typedef DWORD LPTHREAD_START_ROUTINE(LPVOID lpParameter);

    typedef struct tagWNDCLASSA {
        UINT      style;
        WNDPROC   lpfnWndProc;
        int       cbClsExtra;
        int       cbWndExtra;
        HINSTANCE hInstance;
        HICON     hIcon;
        HCURSOR   hCursor;
        HBRUSH    hbrBackground;
        LPCSTR    lpszMenuName;
        LPCSTR    lpszClassName;
    } WNDCLASSA, *PWNDCLASSA, *NPWNDCLASSA, *LPWNDCLASSA;

    typedef struct tagPIXELFORMATDESCRIPTOR {
        WORD  nSize;
        WORD  nVersion;
        DWORD dwFlags;
        BYTE  iPixelType;
        BYTE  cColorBits;
        BYTE  cRedBits;
        BYTE  cRedShift;
        BYTE  cGreenBits;
        BYTE  cGreenShift;
        BYTE  cBlueBits;
        BYTE  cBlueShift;
        BYTE  cAlphaBits;
        BYTE  cAlphaShift;
        BYTE  cAccumBits;
        BYTE  cAccumRedBits;
        BYTE  cAccumGreenBits;
        BYTE  cAccumBlueBits;
        BYTE  cAccumAlphaBits;
        BYTE  cDepthBits;
        BYTE  cStencilBits;
        BYTE  cAuxBuffers;
        BYTE  iLayerType;
        BYTE  bReserved;
        DWORD dwLayerMask;
        DWORD dwVisibleMask;
        DWORD dwDamageMask;
    } PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;

    typedef struct _FILE_NOTIFY_INFORMATION {
        DWORD NextEntryOffset;
        DWORD Action;
        DWORD FileNameLength;
        WCHAR FileName[1];
    } FILE_NOTIFY_INFORMATION, *PFILE_NOTIFY_INFORMATION;

    typedef struct _FILETIME {
        DWORD dwLowDateTime;
        DWORD dwHighDateTime;
    } FILETIME, *PFILETIME, *LPFILETIME;

    typedef struct _WIN32_FIND_DATAA {
        DWORD    dwFileAttributes;
        FILETIME ftCreationTime;
        FILETIME ftLastAccessTime;
        FILETIME ftLastWriteTime;
        DWORD    nFileSizeHigh;
        DWORD    nFileSizeLow;
        DWORD    dwReserved0;
        DWORD    dwReserved1;
        CHAR     cFileName[WIN32_MAX_PATH];
        CHAR     cAlternateFileName[14];
        DWORD    dwFileType;
        DWORD    dwCreatorType;
        WORD     wFinderFlags;
    } WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

    DWORD GetFullPathNameA(
        LPCSTR lpFileName,
        DWORD  nBufferLength,
        LPSTR  lpBuffer,
        LPSTR  *lpFilePart);

    typedef struct tagPOINT {
        LONG x;
        LONG y;
    } POINT, *PPOINT, *NPPOINT, *LPPOINT;

    typedef struct tagMSG {
        HWND   hwnd;
        UINT   message;
        WPARAM wParam;
        LPARAM lParam;
        DWORD  time;
        POINT  pt;
        DWORD  lPrivate;
    } MSG, *PMSG, *NPMSG, *LPMSG;

    typedef union _LARGE_INTEGER {
        struct {
            DWORD LowPart;
            LONG  HighPart;
        } DUMMYSTRUCTNAME;
        struct {
            DWORD LowPart;
            LONG  HighPart;
        } u;
        LONGLONG QuadPart;
    } LARGE_INTEGER, *PLARGE_INTEGER;

    typedef struct _OVERLAPPED {
        ULONG_PTR Internal;
        ULONG_PTR InternalHigh;
        union {
            struct {
                DWORD Offset;
                DWORD OffsetHigh;
            } DUMMYSTRUCTNAME;
            PVOID Pointer;
        } DUMMYUNIONNAME;
        HANDLE    hEvent;
    } OVERLAPPED, *LPOVERLAPPED;

    typedef void LPOVERLAPPED_COMPLETION_ROUTINE(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

    typedef struct _SECURITY_ATTRIBUTES {
        DWORD  nLength;
        LPVOID lpSecurityDescriptor;
        BOOL   bInheritHandle;
    } SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

    typedef struct tagRECT {
        LONG left;
        LONG top;
        LONG right;
        LONG bottom;
    } RECT, *PRECT, *NPRECT, *LPRECT;

    typedef struct _STARTUPINFOW {
        DWORD  cb;
        LPWSTR lpReserved;
        LPWSTR lpDesktop;
        LPWSTR lpTitle;
        DWORD  dwX;
        DWORD  dwY;
        DWORD  dwXSize;
        DWORD  dwYSize;
        DWORD  dwXCountChars;
        DWORD  dwYCountChars;
        DWORD  dwFillAttribute;
        DWORD  dwFlags;
        WORD   wShowWindow;
        WORD   cbReserved2;
        LPBYTE lpReserved2;
        HANDLE hStdInput;
        HANDLE hStdOutput;
        HANDLE hStdError;
    } STARTUPINFOW, *LPSTARTUPINFOW;

    typedef struct _PROCESS_INFORMATION {
        HANDLE hProcess;
        HANDLE hThread;
        DWORD  dwProcessId;
        DWORD  dwThreadId;
    } PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

    // shell32.lib
#define BIF_NEWDIALOGSTYLE (0x00000040)
#define BIF_EDITBOX (0x00000010)
#define BIF_USENEWUI (BIF_EDITBOX|BIF_NEWDIALOGSTYLE)
#define BIF_RETURNONLYFSDIRS (0x00000001)

    typedef int GPFIDL_FLAGS;

    typedef struct _SHITEMID {
        USHORT cb;
        BYTE   abID[1];
    } SHITEMID;

    typedef struct _ITEMIDLIST {
        SHITEMID mkid;
    } ITEMIDLIST, ITEMIDLIST_ABSOLUTE;

    typedef ITEMIDLIST_ABSOLUTE* PIDLIST_ABSOLUTE;
    typedef const ITEMIDLIST_ABSOLUTE* PCIDLIST_ABSOLUTE;


    typedef int ( CALLBACK *BFFCALLBACK)(
        HWND   hwnd,
        UINT   uMsg,
        LPARAM lParam,
        LPARAM lpData);

    typedef struct _browseinfoW {
        HWND              hwndOwner;
        PCIDLIST_ABSOLUTE pidlRoot;
        LPWSTR            pszDisplayName;
        LPCWSTR           lpszTitle;
        UINT              ulFlags;
        BFFCALLBACK       lpfn;
        LPARAM            lParam;
        int               iImage;
    } BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW;

    typedef struct _SYSTEM_INFO {
        union {
            DWORD dwOemId;
            struct {
                WORD wProcessorArchitecture;
                WORD wReserved;
            } DUMMYSTRUCTNAME;
        } DUMMYUNIONNAME;
        DWORD     dwPageSize;
        LPVOID    lpMinimumApplicationAddress;
        LPVOID    lpMaximumApplicationAddress;
        DWORD_PTR dwActiveProcessorMask;
        DWORD     dwNumberOfProcessors;
        DWORD     dwProcessorType;
        DWORD     dwAllocationGranularity;
        WORD      wProcessorLevel;
        WORD      wProcessorRevision;
    } SYSTEM_INFO, *LPSYSTEM_INFO;

    LPWSTR* CommandLineToArgvW(
        LPCWSTR lpCmdLine,
        int *pNumArgs);

    PIDLIST_ABSOLUTE SHBrowseForFolderW(LPBROWSEINFOW lpbi);

    BOOL SHGetPathFromIDListEx(
        PCIDLIST_ABSOLUTE pidl,
        PWSTR             pszPath,
        DWORD             cchPath,
        GPFIDL_FLAGS      uOptss);

    // kernel32.lib
    HMODULE LoadLibraryA(LPCSTR lpLibFileName);
    FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

    LPVOID VirtualAlloc(
        LPVOID lpAddress,
        SIZE_T dwSize,
        DWORD  flAllocationType,
        DWORD  flProtect);

    void GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);

    HANDLE CreateThread(
        LPSECURITY_ATTRIBUTES   lpThreadAttributes,
        SIZE_T                  dwStackSize,
        LPTHREAD_START_ROUTINE  lpStartAddress,
        LPVOID                  lpParameter,
        DWORD                   dwCreationFlags,
        LPDWORD                 lpThreadId);

    BOOL CreatePipe(
        PHANDLE               hReadPipe,
        PHANDLE               hWritePipe,
        LPSECURITY_ATTRIBUTES lpPipeAttributes,
        DWORD                 nSize);

    BOOL SetHandleInformation(
        HANDLE hObject,
        DWORD  dwMask,
        DWORD  dwFlags);

    BOOL CreateProcessW(
        LPCWSTR               lpApplicationName,
        LPWSTR                lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        BOOL                  bInheritHandles,
        DWORD                 dwCreationFlags,
        LPVOID                lpEnvironment,
        LPCWSTR               lpCurrentDirectory,
        LPSTARTUPINFOW        lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation);

    BOOL GetExitCodeProcess(
        HANDLE  hProcess,
        LPDWORD lpExitCode);

    HANDLE CreateMutexA(
        LPSECURITY_ATTRIBUTES lpMutexAttributes,
        BOOL                  bInitialOwner,
        LPCSTR                lpName);

    DWORD WaitForSingleObject(
        HANDLE hHandle,
        DWORD  dwMilliseconds);

    BOOL ReleaseMutex(HANDLE hMutex);

    void Sleep(DWORD dwMilliseconds);

    DWORD GetLastError();

    DWORD FormatMessageA(
        DWORD dwFlags,
        LPCVOID lpSource,
        DWORD dwMessageId,
        DWORD dwLanguageId,
        LPSTR lpBuffer,
        DWORD nSize,
        va_list *Arguments);

    void OutputDebugStringA(LPCSTR lpOutputString);
    void OutputDebugStringW(LPWSTR lpOutputString);

    BOOL IsDebuggerPresent();

    BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
    BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);

    DWORD GetCurrentDirectoryW(DWORD nBufferLength,LPWSTR lpBuffer);
    BOOL SetCurrentDirectoryW(LPCWSTR lpPathName);

    BOOL ReadDirectoryChangesW(
        HANDLE                          hDirectory,
        LPVOID                          lpBuffer,
        DWORD                           nBufferLength,
        BOOL                            bWatchSubtree,
        DWORD                           dwNotifyFilter,
        LPDWORD                         lpBytesReturned,
        LPOVERLAPPED                    lpOverlapped,
        LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

    HANDLE FindFirstFileA(
        LPCSTR             lpFileName,
        LPWIN32_FIND_DATAA lpFindFileData);

    BOOL FindNextFileA(
        HANDLE             hFindFile,
        LPWIN32_FIND_DATAA lpFindFileData);

    BOOL FindClose(HANDLE hFindFile);


    DWORD GetModuleFileNameA(
        HMODULE hModule,
        LPSTR   lpFilename,
        DWORD   nSize);

    DWORD GetModuleFileNameW(
        HMODULE hModule,
        LPWSTR  lpFilename,
        DWORD   nSize);

    LPVOID VirtualAlloc(
        LPVOID lpAddress,
        SIZE_T dwSize,
        DWORD  flAllocationType,
        DWORD  flProtect);

    DECLSPEC_ALLOCATOR HGLOBAL GlobalAlloc(UINT uFlags, SIZE_T dwBytes);

    LPVOID GlobalLock(HGLOBAL hMem);
    BOOL GlobalUnlock(HGLOBAL hMem);

    BOOL CloseHandle(HANDLE hObject);

    HANDLE CreateFileA(
        LPCSTR                lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile);

    BOOL DeleteFileA(LPCSTR lpFileName);

    BOOL WriteFile(
        HANDLE       hFile,
        LPCVOID      lpBuffer,
        DWORD        nNumberOfBytesToWrite,
        LPDWORD      lpNumberOfBytesWritten,
        LPOVERLAPPED lpOverlapped);

    BOOL ReadFile(
        HANDLE       hFile,
        LPVOID       lpBuffer,
        DWORD        nNumberOfBytesToRead,
        LPDWORD      lpNumberOfBytesRead,
        LPOVERLAPPED lpOverlapped);

    DWORD GetFileAttributesA(LPCSTR lpFileName);
    BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
    BOOL CreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);

    BOOL GetFileTime(
        HANDLE     hFile,
        LPFILETIME lpCreationTime,
        LPFILETIME lpLastAccessTime,
        LPFILETIME lpLastWriteTime);

    void GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime);

    HMODULE GetModuleHandleA(LPCSTR lpModuleName);

}

#endif // WIN32_LITE_H
