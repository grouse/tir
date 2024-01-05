#ifndef WIN32_GDI32_H
#define WIN32_GDI32_H

#include "win32_lite.h"

extern "C" {
    
    BOOL SwapBuffers(HDC);
    int ChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd);
    BOOL SetPixelFormat(HDC hdc, int format, const PIXELFORMATDESCRIPTOR *ppfd);
    int DescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd);
}

#endif // WIN32_GDI32_H