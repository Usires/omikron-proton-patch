/*
 * Forwarder-patch.dll für Omikron: The Nomad Soul
 *
 * Diese DLL exportiert die zwei Funktionen die Runtime.exe importiert
 * (DirectDrawCreate, DirectDrawEnumerateA) und leitet sie an Wine's
 * eigene ddraw.dll weiter. Damit sieht das Spiel aus als ob unsere
 * DLL funktioniert, aber Wine's moderner DirectDraw-Stack übernimmt.
 *
 * Compile:
 *   i686-w64-mingw32-gcc -shared -Wl,--image-base=0x10000000 \
 *     -Wl,-u,_DirectDrawCreate@12 -Wl,-u,_DirectDrawEnumerateA@8 \
 *     -o patch.dll dummy_patch.c
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

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}
