#ifndef WIN32_SHLWAPI_H
#define WIN32_SHLWAPI_H

#include "win32_lite.h"

extern "C" {
    BOOL PathFileExistsA(LPCSTR pszPath);
}

#endif // WIN32_SHLWAPI_H
