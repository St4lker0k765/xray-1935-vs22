#include "stdafx.h"
#pragma hdrstop

#include "xrdebug.h"
#include "resource.h"
#include "dbghelp.h"
#include <new>

#include "dxerr9.h"

#ifdef __BORLANDC__
#include "d3d9.h"
#include "d3dx9.h"
#include "D3DX_Wrapper.h"
#pragma comment     (lib,"EToolsB.lib")
static BOOL         bException = TRUE;
#else
static BOOL         bException = FALSE;
#endif

#ifdef _M_AMD64
#define DEBUG_INVOKE    DebugBreak()
#else
#define DEBUG_INVOKE    __debugbreak();
#ifndef __BORLANDC__
#pragma comment     (lib,"dxerr9.lib")
#endif
#endif

extern "C" int __vsnwprintf(wchar_t* buffer, size_t count, const wchar_t* format, va_list argptr) {
    return _vsnwprintf(buffer, count, format, argptr);
}

XRCORE_API xrDebug   Debug;

static const char* dlgExpr = NULL;
static const char* dlgFile = NULL;
static char         dlgLine[16];

static INT_PTR CALLBACK DialogProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG:
    {
        if (dlgFile)
        {
            SetWindowText(GetDlgItem(hw, IDC_DESC), dlgExpr);
            SetWindowText(GetDlgItem(hw, IDC_FILE), dlgFile);
            SetWindowText(GetDlgItem(hw, IDC_LINE), dlgLine);
        }
        else {
            SetWindowText(GetDlgItem(hw, IDC_DESC), dlgExpr);
            SetWindowText(GetDlgItem(hw, IDC_FILE), "");
            SetWindowText(GetDlgItem(hw, IDC_LINE), "");
        }
    }
    break;
    case WM_DESTROY:
        break;
    case WM_CLOSE:
        EndDialog(hw, IDC_STOP);
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == IDC_STOP) {
            EndDialog(hw, IDC_STOP);
        }
        if (LOWORD(wp) == IDC_DEBUG) {
            EndDialog(hw, IDC_DEBUG);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void xrDebug::backend(const char* reason, const char* file, int line)
{
    static xrCriticalSection CS;

    CS.Enter();

    string1024          tmp;
    sprintf(tmp, "***STOP*** file '%s', line %d.\n***Reason***: %s", file, line, reason);
    Msg(tmp);
    FlushLog();
    if (handler)        
        handler();

    dlgExpr = reason;
    dlgFile = file;
    sprintf(dlgLine, "%d", line);
    INT_PTR res = -1;
#ifdef XRCORE_STATIC
    MessageBox(NULL, tmp, "X-Ray error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
#else
    res = DialogBox
    (
        GetModuleHandle(MODULE_NAME),
        MAKEINTRESOURCE(IDD_STOP),
        NULL,
        DialogProc
    );
#endif
    switch (res)
    {
    case -1:
    case IDC_STOP:
        if (bException)     TerminateProcess(GetCurrentProcess(), 3);
        else                RaiseException(0, 0, 0, NULL);
        break;
    case IDC_DEBUG:
        DEBUG_INVOKE;
        break;
    }

    CS.Leave();
}

LPCSTR xrDebug::error2string(long code)
{
    LPCSTR              result = 0;
    static  string1024  desc_storage;

#ifdef _M_AMD64
#else
    result = DXGetErrorDescription9(code);
#endif
    if (0 == result)
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, code, 0, desc_storage, sizeof(desc_storage) - 1, 0);
        result = desc_storage;
    }
    return      result;
}

void xrDebug::error(long hr, const char* expr, const char* file, int line)
{
    string1024  reason;
    sprintf(reason, "*** API-failure ***\n%s\nExpression: %s", error2string(hr), expr);
    backend(reason, file, line);
}

void xrDebug::fail(const char* e1, const char* file, int line)
{
    string1024  reason;
    sprintf(reason, "*** Assertion failed ***\nExpression: %s\n", e1);
    backend(reason, file, line);
}

void xrDebug::fail(const char* e1, const char* e2, const char* file, int line)
{
    string1024  reason;
    sprintf(reason, "*** Assertion failed ***\nExpression: %s\n%s", e1, e2);
    backend(reason, file, line);
}

void xrDebug::fail(const char* e1, std::string &e2, const char* file, int line)
{
    string1024  reason;
    sprintf(reason, "*** Assertion failed ***\nExpression: %s\n%s", e1, e2);
    backend(reason, file, line);
}

void xrDebug::fail(const char* e1, const char* e2, const char* e3, const char* file, int line)
{
    string1024  reason;
    sprintf(reason, "*** Assertion failed ***\nExpression: %s\n%s\n%s", e1, e2, e3);
    backend(reason, file, line);
}
void __cdecl xrDebug::fatal(const char* F, ...)
{
    string1024  buffer;
    string1024  reason;

    va_list     p;
    va_start(p, F);
    vsprintf(buffer, F, p);
    va_end(p);

    sprintf(reason, "*** Fatal Error ***\n%s", buffer);
    backend(reason, 0, 0);
}
int __cdecl _out_of_memory(size_t size)
{
    Debug.fatal("Out of memory. Memory request: %d K", size / 1024);
    return                  1;
}
void __cdecl _terminate()
{
    Debug.fatal("Unexpected application termination");
}

typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

LONG WINAPI UnhandledFilter(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
    LONG retval = EXCEPTION_CONTINUE_SEARCH;
    bException = TRUE;

    HMODULE hDll = NULL;
    string_path     szDbgHelpPath;

    if (GetModuleFileName(NULL, szDbgHelpPath, _MAX_PATH))
    {
        char* pSlash = strchr(szDbgHelpPath, '\\');
        if (pSlash)
        {
            strcpy(pSlash + 1, "DBGHELP.DLL");
            hDll = ::LoadLibrary(szDbgHelpPath);
        }
    }

    if (hDll == NULL)
    {
        hDll = ::LoadLibrary("DBGHELP.DLL");
    }

    LPCTSTR szResult = NULL;

    if (hDll)
    {
        MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
        if (pDump)
        {
            string_path szDumpPath;
            string_path szScratch;
            string64    t_stemp;

            timestamp(t_stemp);
            strcpy(szDumpPath, "logs\\");
            strcat(szDumpPath, Core.ApplicationName);
            strcat(szDumpPath, "_");
            strcat(szDumpPath, Core.UserName);
            strcat(szDumpPath, "_");
            strcat(szDumpPath, t_stemp);
            strcat(szDumpPath, ".mdmp");

            HANDLE hFile = ::CreateFile(szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (INVALID_HANDLE_VALUE == hFile)
            {
                MoveMemory(szDumpPath, szDumpPath + 5, strlen(szDumpPath));
                hFile = ::CreateFile(szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            if (hFile != INVALID_HANDLE_VALUE)
            {
                _MINIDUMP_EXCEPTION_INFORMATION ExInfo;

                ExInfo.ThreadId = ::GetCurrentThreadId();
                ExInfo.ExceptionPointers = pExceptionInfo;
                ExInfo.ClientPointers = NULL;

                MINIDUMP_TYPE   dump_flags = MINIDUMP_TYPE(MiniDumpNormal | MiniDumpFilterMemory | MiniDumpScanMemory);

                BOOL bOK = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dump_flags, &ExInfo, NULL, NULL);
                if (bOK)
                {
                    sprintf(szScratch, "Saved dump file to '%s'", szDumpPath);
                    szResult = szScratch;
                    retval = EXCEPTION_EXECUTE_HANDLER;
                }
                else
                {
                    sprintf(szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, GetLastError());
                    szResult = szScratch;
                }
                ::CloseHandle(hFile);
            }
            else
            {
                sprintf(szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, GetLastError());
                szResult = szScratch;
            }
        }
        else
        {
            szResult = "DBGHELP.DLL too old";
        }
    }
    else
    {
        szResult = "DBGHELP.DLL not found";
    }

    string1024      reason;
    sprintf(reason, "*** Internal Error ***\n%s", szResult);
    Debug.backend(reason, 0, 0);

    return retval;
}

#ifdef M_BORLAND
namespace std {
    extern new_handler _RTLENTRY _EXPFUNC set_new_handler(new_handler new_p);
};
static void __cdecl def_new_handler()
{
    Debug.fatal("Out of memory.");
}
void    xrDebug::_initialize()
{
    std::set_new_handler(def_new_handler);
    ::SetUnhandledExceptionFilter(UnhandledFilter);
}
#else
static void __cdecl def_new_handler()
{
    _out_of_memory(static_cast<size_t>(~0u));
}
void    xrDebug::_initialize()
{
    handler = 0;
    std::set_new_handler(def_new_handler);
    std::set_terminate(_terminate);
    //std::set_unexpected(_terminate);
    ::SetUnhandledExceptionFilter(UnhandledFilter);
}
#endif

void xrDebug::do_exit	(const std::string &message)
{
	FlushLog			();
    MessageBox			(NULL,message.c_str(),"Error",MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
    TerminateProcess	(GetCurrentProcess(),1);
}
