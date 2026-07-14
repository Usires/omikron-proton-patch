/*
 * forwarder.c — DirectDraw forwarder DLL for Omikron: The Nomad Soul (GOG).
 *
 * The GOG release of Omikron ships a small indirection layer: Runtime.exe
 * imports DirectDrawCreate and DirectDrawEnumerateA from patch.dll, not
 * directly from ddraw.dll. This file is the source for the replacement
 * patch.dll that ships in this repository.
 *
 * The two exported functions forward their calls to Wine's built-in
 * ddraw.dll via LoadLibraryA + GetProcAddress. Wine's modern DirectDraw
 * stack handles the actual work, layered on top of dgVoodoo2 (D3D -> Vulkan)
 * and DSOAL (DirectSound3D -> OpenAL).
 *
 * Build via ./build.sh (which cross-compiles with i686-w64-mingw32-gcc):
 *   i686-w64-mingw32-gcc -shared -Wl,--image-base=0x10000000 \
 *     -Wl,-u,_DirectDrawCreate@12 -Wl,-u,_DirectDrawEnumerateA@8 \
 *     -o patch.dll src/forwarder.c
 *
 * The build script additionally:
 * - Bypasses the CRT startup wrapper (--entry=_DllMainCRTStartup is the
 *   default; we override with --entry=DllMain) so the binary contains
 *   only our forwarder code.
 * - Strips debug info (-g0, --strip-debug) and unwind tables
 *   (-fno-asynchronous-unwind-tables).
 * - Garbage-collects unused sections (--gc-sections).
 *
 * Result: a ~5 KB DLL, matching the size of the GOG-shipped patch.dll
 * that uses the same minimal forwarder pattern.
 */
#include <windows.h>

/* Wine's ddraw.dll ist ein builtin. Wir können sie nicht direkt linken,
 * aber wir können LoadLibrary + GetProcAddress nutzen. */

/* Original-Funktionspointer-Typen aus ddraw.h */
typedef HRESULT (WINAPI *PFN_DirectDrawCreate)(GUID *lpGUID, LPVOID *lplpDD, IUnknown *pUnkOuter);

typedef struct _DDENUMCALLBACKA {
    BOOL (CALLBACK *callback)(GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext);
} DDENUMCALLBACKA_SHIM;

typedef BOOL (CALLBACK *PFN_DDENUMCALLBACKA)(GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext);
typedef HRESULT (WINAPI *PFN_DirectDrawEnumerateA)(PFN_DDENUMCALLBACKA lpCallback, LPVOID lpContext);

#define DDENUMRET_CANCEL 1
#define DDENUMRET_OK 0

/* Wine's ddraw.dll ist in C:\\windows\\system32\\ddraw.dll. In Wine gibt's
 * zwei Versionen: 32-bit (in syswow64) und 64-bit (in system32). Das Spiel
 * ist 32-bit, also brauchen wir syswow64. */
static HMODULE g_ddraw = NULL;

static HMODULE get_ddraw(void) {
    if (g_ddraw) return g_ddraw;
    /* Lazy load: erst wenn die erste Funktion aufgerufen wird */
    g_ddraw = LoadLibraryA("ddraw.dll");
    return g_ddraw;
}

HRESULT WINAPI DirectDrawCreate(
    GUID *lpGUID,
    LPVOID *lplpDD,
    IUnknown *pUnkOuter)
{
    HMODULE h = get_ddraw();
    if (!h) return E_FAIL;
    PFN_DirectDrawCreate fn = (PFN_DirectDrawCreate)GetProcAddress(h, "DirectDrawCreate");
    if (!fn) return E_FAIL;
    return fn(lpGUID, lplpDD, pUnkOuter);
}

HRESULT WINAPI DirectDrawEnumerateA(
    PFN_DDENUMCALLBACKA lpCallback,
    LPVOID lpContext)
{
    HMODULE h = get_ddraw();
    if (!h) return E_FAIL;
    PFN_DirectDrawEnumerateA fn = (PFN_DirectDrawEnumerateA)GetProcAddress(h, "DirectDrawEnumerateA");
    if (!fn) return E_FAIL;
    return fn(lpCallback, lpContext);
}

/* Entry point. With -Wl,--entry=DllMain (undecorated), the loader calls
 * this directly, bypassing _DllMainCRTStartup. We do our own minimal
 * init: disable thread-library calls (we don't need them). */
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    (void)hinstDLL;
    (void)lpvReserved;
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}
