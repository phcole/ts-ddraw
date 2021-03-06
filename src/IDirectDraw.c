/*
 * Copyright (c) 2013 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "main.h"
#include "IDirectDraw.h"
#include "IDirectDrawClipper.h"
#include "IDirectDrawSurface.h"

 // use these to enable stretching for testing
 // works only fullscreen right now

bool MaintainAspectRatio = false;
bool Windowboxing = false;
bool StretchToFullscreen = false;
int StretchToWidth = 0;
int StretchToHeight = 0;

static IDirectDrawImplVtbl Vtbl;
static IDirectDrawImpl *ddraw;

IDirectDrawImpl *IDirectDrawImpl_construct()
{
    dprintf("--> IDirectDraw::construct()\n");

    IDirectDrawImpl *this = calloc(1, sizeof(IDirectDrawImpl));
    this->lpVtbl = &Vtbl;
    this->dd = this;

    this->ref++;
    timeBeginPeriod(1);
    ddraw = this;

    this->glInfo.glSupported = false;
    this->glInfo.initialized = false;
    this->glInfo.pboSupported = false;

    dprintf("<-- IDirectDraw::construct() -> %p\n", this);
    return this;
}

static HRESULT __stdcall _QueryInterface(IDirectDrawImpl *this, REFIID riid, void **obj)
{
    ENTER;
    dprintf("--> IDirectDraw::QueryInterface(this=%p, riid=%p, obj=%p)\n", this, riid, obj);
    HRESULT ret = IDirectDraw_QueryInterface(this->real, riid, obj);
    dprintf("<-- IDirectDraw::QueryInterface(this=%p, riid=%p, obj=%p) -> %08X\n", this, riid, obj, (int)ret);
    LEAVE;
    return ret;
}

static ULONG __stdcall _AddRef(IDirectDrawImpl *this)
{
    ENTER;
    dprintf("--> IDirectDraw::AddRef(this=%p)\n", this);
    ULONG ret = IDirectDraw_AddRef(this->real);
    dprintf("<-- IDirectDraw::AddRef(this=%p) -> %08X\n", this, (int)ret);
    LEAVE;
    return ret;
}

static ULONG __stdcall _Release(IDirectDrawImpl *this)
{
    ENTER;
    dprintf("--> IDirectDraw::Release(this=%p)\n", this);
    ULONG ret = --this->ref;

    if (PROXY)
    {
        ret = IDirectDraw_Release(this->real);
    }
    else
    {
        if (this->ref == 0)
        {
            timeEndPeriod(1);
            free(this);
        }
    }

    dprintf("<-- IDirectDraw::Release(this=%p) -> %08X\n", this, (int)ret);
    LEAVE;
    return ret;
}

static HRESULT __stdcall _Compact(IDirectDrawImpl *this)
{
    dprintf("--> IDirectDraw::Compact(this=%p)\n", this);
    HRESULT ret = IDirectDraw_Compact(this->real);
    dprintf("<-- IDirectDraw::Compact(this=%p) -> %08X\n", this, (int)ret);
    return ret;
}

static HRESULT __stdcall _CreatePalette(IDirectDrawImpl *this, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR * lplpDDPalette, IUnknown FAR * pUnkOuter)
{
    dprintf(
        "--> IDirectDraw::CreatePalette(this=%p, dwFlags=%d, lpDDColorArray=%p, lplpDDPalette=%p, pUnkOuter=%p)\n",
        this, (int)dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter);

    HRESULT ret = IDirectDraw_CreatePalette(this->real, dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter);

    dprintf(
        "<-- IDirectDraw::CreatePalette(this=%p, dwFlags=%d, lpDDColorArray=%p, lplpDDPalette=%p, pUnkOuter=%p) -> %08X\n",
        this, (int)dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter, (int)ret);
    return ret;
}

static HRESULT __stdcall _CreateClipper(IDirectDrawImpl *this, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
    ENTER;
    dprintf("--> IDirectDraw::CreateClipper(this=%p, dwFlags=%d, lplpDDClipper=%p, unkOuter=%p)\n", this, (int)dwFlags, lplpDDClipper, pUnkOuter);

    HRESULT ret = DD_OK;
    IDirectDrawClipperImpl *impl = IDirectDrawClipperImpl_construct();
    *lplpDDClipper = (IDirectDrawClipper *)impl;

    if (PROXY)
    {
        ret = IDirectDraw_CreateClipper(this->real, dwFlags, &impl->real, pUnkOuter);
    }

    dprintf("<-- IDirectDraw::CreateClipper(this=%p, dwFlags=%d, lplpDDClipper=%p, unkOuter=%p) -> %08X\n", this, (int)dwFlags, lplpDDClipper, pUnkOuter, (int)ret);
    LEAVE;
    return ret;
}

static HRESULT __stdcall _CreateSurface(IDirectDrawImpl *this, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR * pUnkOuter)
{
    ENTER;
    dprintf("--> IDirectDraw::CreateSurface(this=%p, lpDDSurfaceDesc=%p, lplpDDSurface=%p, pUnkOuter=%p)\n", this, lpDDSurfaceDesc, lplpDDSurface, pUnkOuter);

    HRESULT ret = DD_OK;
    IDirectDrawSurfaceImpl *impl = IDirectDrawSurfaceImpl_construct(this, lpDDSurfaceDesc);
    *lplpDDSurface = (IDirectDrawSurface *)impl;

    if (PROXY)
    {
        ret = IDirectDraw_CreateSurface(this->real, lpDDSurfaceDesc, &impl->real, pUnkOuter);
    }

    if (VERBOSE)
    {
        dprintf("    Surface description:\n");
        dump_ddsurfacedesc(lpDDSurfaceDesc);
    }

    dprintf("<-- IDirectDraw::CreateSurface(this=%p, lpDDSurfaceDesc=%p, lplpDDSurface=%p, pUnkOuter=%p) -> %08X\n", this, lpDDSurfaceDesc, lplpDDSurface, pUnkOuter, (int)ret);
    LEAVE;
    return ret;
}

static HRESULT __stdcall _DuplicateSurface(IDirectDrawImpl *this, LPDIRECTDRAWSURFACE src, LPDIRECTDRAWSURFACE *dest)
{
    dprintf("--> IDirectDraw::DuplicateSurface(this=%p, src=%p, dest=%p)\n", this, src, dest);
    HRESULT ret = DDERR_UNSUPPORTED;

    if (PROXY)
    {
        ret = IDirectDraw_DuplicateSurface(this->real, src, dest);
    }

    dprintf("<-- IDirectDraw::DuplicateSurface(this=%p, src=%p, dest=%p) -> %08X\n", this, src, dest, (int)ret);
    return ret;
}

static HRESULT __stdcall _EnumDisplayModes(IDirectDrawImpl *this, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
    dprintf(
        "--> IDirectDraw::EnumDisplayModes(this=%p, dwFlags=%08X, lpDDSurfaceDesc=%p, lpContext=%p, lpEnumModesCallback=%p)\n",
        this, (int)dwFlags, lpDDSurfaceDesc, lpContext, lpEnumModesCallback);

    HRESULT ret = DD_OK;

    if (PROXY)
    {
        ret = IDirectDraw_EnumDisplayModes(this->real, dwFlags, lpDDSurfaceDesc, lpContext, lpEnumModesCallback);
    }
    else
    {
        DEVMODE mode;
        mode.dmSize = sizeof(mode);

        int i = 0;
        while (EnumDisplaySettings(NULL, i, &mode))
        {
            // enumerate desktop bpp modes
            if (mode.dmBitsPerPel == this->winMode.dmBitsPerPel)
            {
                DDSURFACEDESC desc;
                memset(&desc, 0, sizeof(desc));
                desc.dwSize = sizeof(desc);
                desc.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_REFRESHRATE|DDSD_PIXELFORMAT;
                desc.dwWidth = mode.dmPelsWidth;
                desc.dwHeight = mode.dmPelsHeight;
                desc.dwRefreshRate = mode.dmDisplayFrequency;
                desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
                desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
                desc.ddpfPixelFormat.dwRGBBitCount = this->bpp;

                lpEnumModesCallback(&desc, lpContext);
            }
            i++;
        }
    }

    dprintf(
        "<-- IDirectDraw::EnumDisplayModes(this=%p, dwFlags=%08X, lpDDSurfaceDesc=%p, lpContext=%p, lpEnumModesCallback=%p) -> %08X\n",
        this, (int)dwFlags, lpDDSurfaceDesc, lpContext, lpEnumModesCallback, (int)ret);

    return ret;
}

static HRESULT __stdcall _EnumSurfaces(IDirectDrawImpl *this, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
    dprintf(
        "--> IDirectDraw::EnumSurfaces(this=%p, dwFlags=%08X, lpDDSD=%p, lpContext=%p, lpEnumSurfacesCallback=%p)\n",
        this, (int)dwFlags, lpDDSD, lpContext, lpEnumSurfacesCallback);

    HRESULT ret = IDirectDraw_EnumSurfaces(this->real, dwFlags, lpDDSD, lpContext, lpEnumSurfacesCallback);

    dprintf(
        "<-- IDirectDraw::EnumSurfaces(this=%p, dwFlags=%08X, lpDDSD=%p, lpContext=%p, lpEnumSurfacesCallback=%p) -> %08X\n",
        this, (int)dwFlags, lpDDSD, lpContext, lpEnumSurfacesCallback, (int)ret);

    return ret;
}

static HRESULT __stdcall _FlipToGDISurface(IDirectDrawImpl *this)
{
    dprintf("--> IDirectDraw::FlipToGDISurface(this=%p)\n", this);
    HRESULT ret = IDirectDraw_FlipToGDISurface(this->real);
    dprintf("<-- IDirectDraw::FlipToGDISurface(this=%p) -> %08X\n", this, (int)ret);
    return ret;
}

static HRESULT __stdcall _GetCaps(IDirectDrawImpl *this, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDEmulCaps)
{
    dprintf(
        "--> IDirectDraw::GetCaps(this=%p, lpDDDriverCaps=%p, lpDDEmulCaps=%p)\n",
        this, lpDDDriverCaps, lpDDEmulCaps);


    HRESULT ret = DD_OK;

    if (PROXY)
    {
        ret = IDirectDraw_GetCaps(this->real, lpDDDriverCaps, lpDDEmulCaps);
    }
    else
    {
        if (lpDDDriverCaps)
        {
            lpDDDriverCaps->dwSize = sizeof(DDCAPS);
            lpDDDriverCaps->dwCaps = DDCAPS_BLT|DDCAPS_PALETTE;
            lpDDDriverCaps->dwCKeyCaps = 0;
            lpDDDriverCaps->dwPalCaps = DDPCAPS_8BIT|DDPCAPS_PRIMARYSURFACE;
            lpDDDriverCaps->dwVidMemTotal = 16777216;
            lpDDDriverCaps->dwVidMemFree = 16777216;
            lpDDDriverCaps->dwMaxVisibleOverlays = 0xFFFFFFFF;
            lpDDDriverCaps->dwCurrVisibleOverlays = 0xFFFFFFFF;
            lpDDDriverCaps->dwNumFourCCCodes = 0xFFFFFFFF;
            lpDDDriverCaps->dwAlignBoundarySrc = 0;
            lpDDDriverCaps->dwAlignSizeSrc = 0;
            lpDDDriverCaps->dwAlignBoundaryDest = 0;
            lpDDDriverCaps->dwAlignSizeDest = 0;

            lpDDDriverCaps->dwSize = 316;
            lpDDDriverCaps->dwCaps = 0xF5408669;
            lpDDDriverCaps->dwCaps2 = 0x000A1801;
        }

        if (lpDDEmulCaps)
        {
            memset(lpDDEmulCaps, 0, sizeof(*lpDDEmulCaps));
            lpDDDriverCaps->dwSize = sizeof(*lpDDEmulCaps);
        }
    }

    if (lpDDDriverCaps && VERBOSE)
    {
        dprintf("    Hardware capabilities:\n");
        dump_ddcaps(lpDDDriverCaps);
    }

    if (lpDDEmulCaps && VERBOSE)
    {
        dprintf("    HEL capabilities:\n");
        dump_ddcaps(lpDDDriverCaps);
    }

    dprintf(
        "<-- IDirectDraw::GetCaps(this=%p, lpDDDriverCaps=%p, lpDDEmulCaps=%p) -> %08X\n",
        this, lpDDDriverCaps, lpDDEmulCaps, (int)ret);

    return ret;
}

static HRESULT __stdcall _GetDisplayMode(IDirectDrawImpl *this, LPDDSURFACEDESC lpDDSurfaceDesc)
{
    dprintf("--> IDirectDraw::GetDisplayMode(this=%p, lpDDSurfaceDesc=%p)\n", this, lpDDSurfaceDesc);
    HRESULT ret = IDirectDraw_GetDisplayMode(this->real, lpDDSurfaceDesc);
    dprintf("<-- IDirectDraw::GetDisplayMode(this=%p, lpDDSurfaceDesc=%p) -> %08X\n", this, lpDDSurfaceDesc, (int)ret);
    return ret;
}

static HRESULT __stdcall _GetFourCCCodes(IDirectDrawImpl *this, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
    dprintf("--> IDirectDraw::GetFourCCCodes(this=%p, lpNumCodes=%p, lpCodes=%p)\n", this, lpNumCodes, lpCodes);
    HRESULT ret = IDirectDraw_GetFourCCCodes(this->real, lpNumCodes, lpCodes);
    dprintf("<-- IDirectDraw::GetFourCCCodes(this=%p, lpNumCodes=%p, lpCodes=%p) -> %08X\n", this, lpNumCodes, lpCodes, (int)ret);
    return ret;
}

static HRESULT __stdcall _GetGDISurface(IDirectDrawImpl *this, LPDIRECTDRAWSURFACE *lplpGDIDDSSurface)
{
    dprintf("--> IDirectDraw::GetGDISurface(this=%p, lplpGDIDDSSurface=%p)\n", this, lplpGDIDDSSurface);
    HRESULT ret = IDirectDraw_GetGDISurface(this->real, lplpGDIDDSSurface);
    dprintf("<-- IDirectDraw::GetGDISurface(this=%p, lplpGDIDDSSurface=%p) -> %08X\n", this, lplpGDIDDSSurface, (int)ret);
    return ret;
}

static HRESULT __stdcall _GetMonitorFrequency(IDirectDrawImpl *this, LPDWORD lpdwFrequency)
{
    dprintf("--> IDirectDraw::GetMonitorFrequency(this=%p, lpdwFrequency=%p)\n", this, lpdwFrequency);
    HRESULT ret = IDirectDraw_GetMonitorFrequency(this->real, lpdwFrequency);
    dprintf("<-- IDirectDraw::GetMonitorFrequency(this=%p, lpdwFrequency=%p) -> %08X\n", this, lpdwFrequency, (int)ret);
    return ret;
}

static HRESULT __stdcall _GetScanLine(IDirectDrawImpl *this, LPDWORD lpdwScanLine)
{
    dprintf("--> IDirectDraw::GetScanLine(this=%p, lpdwScanLine=%p)\n", this, lpdwScanLine);
    HRESULT ret = IDirectDraw_GetScanLine(this->real, lpdwScanLine);
    dprintf("<-- IDirectDraw::GetScanLine(this=%p, lpdwScanLine=%p) -> %08X\n", this, lpdwScanLine, (int)ret);
    return ret;
}

static HRESULT __stdcall _GetVerticalBlankStatus(IDirectDrawImpl *this, LPBOOL lpbIsInVB)
{
    dprintf("--> IDirectDraw::GetVerticalBlankStatus(this=%p, lpbIsInVB=%s)\n", this, (lpbIsInVB ? "TRUE" : "FALSE"));
    HRESULT ret = DDERR_UNSUPPORTED;

    if (PROXY)
    {
        ret = IDirectDraw_GetVerticalBlankStatus(this->real, lpbIsInVB);
    }

    dprintf("<-- IDirectDraw::GetVerticalBlankStatus(this=%p, lpbIsInVB=%s) -> %08X\n", this, (lpbIsInVB ? "TRUE" : "FALSE"), (int)ret);
    return ret;
}

static HRESULT __stdcall _Initialize(IDirectDrawImpl *this, GUID *lpGUID)
{
    dprintf("--> IDirectDraw::Initialize(this=%p, lpGUID=%p)\n", this, lpGUID);

    HRESULT ret = DDERR_UNSUPPORTED;

    if (PROXY)
    {
        ret = IDirectDraw_Initialize(this->real, lpGUID);
    }

    dprintf("<-- IDirectDraw::Initialize(this=%p, lpGUID=%p) -> %08X\n", this, lpGUID, (int)ret);
    return ret;
}

static HRESULT __stdcall _RestoreDisplayMode(IDirectDrawImpl *this)
{
    dprintf("--> IDirectDraw::RestoreDisplayMode(this=%p)\n", this);
    HRESULT ret = DD_OK;

    if (PROXY)
    {
        ret = IDirectDraw_RestoreDisplayMode(this->real);
    }
    else
    {
        ChangeDisplaySettings(&this->winMode, 0);
    }

    dprintf("<-- IDirectDraw::RestoreDisplayMode(this=%p) -> %08X\n", this, (int)ret);
    return ret;
}

void SetWindowSize(IDirectDrawImpl *this, DWORD width, DWORD height)
{
    this->width = width;
    this->height = height;

    this->render.width = StretchToWidth;
    this->render.height = StretchToHeight;

    if (StretchToFullscreen)
    {
        this->render.width = this->screenWidth;
        this->render.height = this->screenHeight;
    }

    if (this->render.width < this->width)
        this->render.width = this->width;

    if (this->render.height < this->height)
        this->render.height = this->height;

    this->render.viewport.width = this->render.width;
    this->render.viewport.height = this->render.height;
    this->render.viewport.x = 0;
    this->render.viewport.y = 0;

    if (Windowboxing)
    {
        this->render.viewport.width = this->width;
        this->render.viewport.height = this->height;

        for (int i = 20; i-- > 1;)
        {
            if (this->width * i <= this->render.width && this->height * i <= this->render.height)
            {
                this->render.viewport.width *= i;
                this->render.viewport.height *= i;
                break;
            }
        }

        this->render.viewport.y = this->render.height / 2 - this->render.viewport.height / 2;
        this->render.viewport.x = this->render.width / 2 - this->render.viewport.width / 2;
    }
    else if (MaintainAspectRatio)
    {
        this->render.viewport.width = this->render.width;
        this->render.viewport.height = ((float)this->height / this->width) * this->render.viewport.width;

        if (this->render.viewport.height > this->render.height)
        {
            this->render.viewport.width =
                ((float)this->render.viewport.width / this->render.viewport.height) * this->render.height;

            this->render.viewport.height = this->render.height;
        }

        this->render.viewport.y = this->render.height / 2 - this->render.viewport.height / 2;
        this->render.viewport.x = this->render.width / 2 - this->render.viewport.width / 2;
    }

    this->render.scaleW = ((float)this->render.viewport.width / this->width);
    this->render.scaleH = ((float)this->render.viewport.height / this->height);

    this->render.stretched =
        this->render.viewport.width != this->width ||
        this->render.viewport.height != this->height ||
        this->render.viewport.x != 0 ||
        this->render.viewport.y != 0;

    // Don't resize the window in full screen mode. This prevents X window managers
    // from moving the window around in full screen mode
    if (this->dwFlags & DDSCL_FULLSCREEN)
        SetWindowPos(this->hWnd, HWND_TOP, 0, 0, this->screenWidth, this->screenHeight, SWP_SHOWWINDOW);
    else
        SetWindowPos(this->hWnd, HWND_TOP, 0, 0, this->render.width, this->render.height, SWP_SHOWWINDOW);

    this->render.invalidate = TRUE;
}

static HRESULT __stdcall _SetDisplayMode(IDirectDrawImpl *this, DWORD width, DWORD height, DWORD bpp)
{
    ENTER;
    dprintf("--> IDirectDraw::SetDisplayMode(this=%p, width=%d, height=%d, bpp=%d)\n", this, (int)width, (int)height, (int)bpp);
    HRESULT ret = DD_OK;

    if (PROXY)
    {
        ret = IDirectDraw_SetDisplayMode(this->real, width, height, bpp);
    }
    else
    {
        if (bpp != 16)
            return DDERR_INVALIDMODE;

        SetWindowSize(this, width, height);

        this->bpp = bpp;

        if (!IsWindowsXp())
        {
            this->pfd.nSize = sizeof(this->pfd);
            this->pfd.nVersion = 1;

            if (InterlockedExchangeAdd(&Renderer, 0) == RENDERER_OPENGL)
                this->pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_SWAP_EXCHANGE;
            else
                this->pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
            this->pfd.iPixelType = PFD_TYPE_RGBA;
            this->pfd.cColorBits = this->bpp;
            this->pfd.iLayerType = PFD_MAIN_PLANE;
            if (!SetPixelFormat(this->hDC, ChoosePixelFormat(this->hDC, &this->pfd), &this->pfd))
            {
                dprintf("SetPixelFormat failed!\n");
                //ret = DDERR_UNSUPPORTED;
            }
        }

        if (IsWine())
            SetWindowLong(this->hWnd, GWL_STYLE, GetWindowLong(this->hWnd, GWL_STYLE) | WS_MINIMIZEBOX);

        int index = 0;
        DEVMODE dm;
        BOOL foundDevMode = false;
        while ( 0 != EnumDisplaySettings(NULL, index++, &dm))
        {
            if ((dm.dmFields & (DM_PELSWIDTH|DM_PELSHEIGHT)) == (DM_PELSWIDTH|DM_PELSHEIGHT))
            {
                if (this->mode.dmBitsPerPel != this->bpp)
                {
                    if (dm.dmPelsWidth == this->render.width && dm.dmPelsHeight == this->render.height)
                    {
                        memcpy(&this->mode, &dm, sizeof(this->mode));
                        foundDevMode = true;
                    }
                }
            }
        }

        if (!foundDevMode)
        {
            this->mode.dmSize = sizeof(this->mode);
            this->mode.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT;
            this->mode.dmPelsWidth = this->render.width;
            this->mode.dmPelsHeight = this->render.height;
        }

        // Only use the full screen hack in wine since it might disrupt OBS and recording software.
        if (IsWine())
            SetWindowPos(this->hWnd, HWND_TOP, 0, 0, this->screenWidth, this->screenHeight, SWP_SHOWWINDOW);
        else
            SetWindowPos(this->hWnd, HWND_TOP, 0, 0, this->render.width, this->render.height, SWP_SHOWWINDOW);

        if (ChangeDisplaySettings(&this->mode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            // odd height or half screenWidth/Height trigger scaling on invalid resolutions (hidden feature)
            if (this->height % 2 != 0 || (this->width * 2 == this->screenWidth && this->height * 2 == this->screenHeight))
            {
                this->mode.dmPelsWidth = StretchToWidth = this->screenWidth;
                this->mode.dmPelsHeight = StretchToHeight = this->screenHeight;

                if (ChangeDisplaySettings(&this->mode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
                    return DDERR_INVALIDMODE;
            }
            else
            {
                dprintf("    mode change failed!\n");
                return DDERR_INVALIDMODE;
            }
        }
        SetWindowPos(ddraw->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW);

        POINT p = { 0, 0 };
        ClientToScreen(this->dd->hWnd, &p);
        GetClientRect(this->dd->hWnd, &this->winRect);
        OffsetRect(&this->winRect, p.x, p.y);

        mouse_lock(this);
    }

    dprintf("<-- IDirectDraw::SetDisplayMode(this=%p, width=%d, height=%d, bpp=%d) -> %08X\n", this, (int)width, (int)height, (int)bpp, (int)ret);
    LEAVE;
    return ret;
}

BOOL WINAPI fake_SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    if (!(uFlags & SWP_NOSIZE) && ddraw && hWnd == ddraw->hWnd)
    {
        SetWindowSize(ddraw, cx, cy);
        return TRUE;
    }

    return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

BOOL WINAPI fake_MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
    if (ddraw && hWnd == ddraw->hWnd)
    {
        RECT rc = { 0, 0, nWidth, nHeight };

        if (UnadjustWindowRectEx(&rc, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE)))
        {
            ddraw->width = rc.right - rc.left;
            ddraw->height = rc.bottom - rc.top;
        }
    }

    return MoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
}

BOOL WINAPI fake_GetCursorPos(LPPOINT lpPoint)
{
    if (InterlockedExchangeAdd(&ddraw->mouseIsLocked, 0) != 0)
        return GetCursorPos(lpPoint);

    BOOL ret = GetCursorPos(lpPoint);

    POINT tl = { 0, 0 };
    POINT br = { ddraw->width, ddraw->height };

    ClientToScreen(ddraw->hWnd, &tl);
    ClientToScreen(ddraw->hWnd, &br);

    if (ret)
    {
        lpPoint->x = lpPoint->x < (tl.x + 3) ? (tl.x + 3) : lpPoint->x;
        lpPoint->x = lpPoint->x > (br.x - 3) ? (br.x - 3) : lpPoint->x;

        lpPoint->y = lpPoint->y < (tl.y + 3) ? (tl.y + 3) : lpPoint->y;
        lpPoint->y = lpPoint->y > (br.y - 3) ? (br.y - 3) : lpPoint->y;
    }

    return ret;
}

BOOL UnadjustWindowRectEx(LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle)
{
    RECT rc;
    SetRectEmpty(&rc);

    BOOL fRc = AdjustWindowRectEx(&rc, dwStyle, fMenu, dwExStyle);
    if (fRc)
    {
        prc->left -= rc.left;
        prc->top -= rc.top;
        prc->right -= rc.right;
        prc->bottom -= rc.bottom;
    }

    return fRc;
}

bool CaptureMouse = false;

void mouse_lock(IDirectDrawImpl *this)
{
    RECT rc;

    GetClientRect(this->hWnd, &rc);

    if (ddraw->render.stretched)
    {
        rc.right = ddraw->width;
        rc.bottom = ddraw->height;
    }

    // Convert the client area to screen coordinates.
    POINT pt = { rc.left, rc.top };
    POINT pt2 = { rc.right, rc.bottom };
    ClientToScreen(this->hWnd, &pt);
    ClientToScreen(this->hWnd, &pt2);

    if (this->dwFlags & DDSCL_FULLSCREEN)
        SetRect(&rc, 0, 0, this->width, this->height);
    else
        SetRect(&rc, pt.x, pt.y, pt2.x, pt2.y);

    ClipCursor(&rc);
    CaptureMouse = true;
    InterlockedExchange(&ddraw->mouseIsLocked, 1);

    //while(ShowCursor(false) >= 0) ;
}

void mouse_unlock(BOOL showCursor)
{
    ClipCursor(NULL);
    InterlockedExchange(&ddraw->mouseIsLocked, 0);

    //if (showCursor)
    //    while(ShowCursor(true) < 0);
}

void center_mouse(HWND hWnd)
{
    RECT size;
    GetClientRect(hWnd, &size);

    if (ddraw->render.stretched)
    {
        size.right = ddraw->width;
        size.bottom = ddraw->height;
    }

    POINT pos = { 0, 0 };
    ClientToScreen(hWnd, &pos);

    SetCursorPos(size.right / 2 + pos.x, size.bottom / 2 + pos.y);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IDirectDrawImpl *this = ddraw;

    static double rememberFPS = -1;
    static DWORD rememberDrawFPS = 0;

    static BOOL fsActive = true;

    switch(uMsg)
    {
        //Workaround for invisible menu on Load/Save/Delete in Tiberian Sun
        static int redrawCount = 0;
        case WM_PARENTNOTIFY:
        {
            if (LOWORD(wParam) == WM_DESTROY)
                redrawCount = 2;
            break;
        }
        case WM_PAINT:
        {
            if (redrawCount > 0)
            {
                redrawCount--;
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            if (InterlockedExchangeAdd(&this->mouseIsLocked, 0) == 0)
                mouse_lock(this);
            break;

        case WM_ACTIVATE:
            if (this->dwFlags & DDSCL_FULLSCREEN)
            {
                if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
                {
                    fsActive = true;
                    ShowWindow(this->hWnd, SW_RESTORE);
                    ChangeDisplaySettings(&this->mode, CDS_FULLSCREEN);
                    mouse_lock(this);
                    InterlockedExchange(&this->dd->focusGained, true);
                }
                else if (wParam == WA_INACTIVE && InterlockedExchangeAdd(&this->mouseIsLocked, 0) != 0)
                {
                    fsActive = false;
                    ShowWindow(this->hWnd, SW_MINIMIZE);
                    ChangeDisplaySettings(&this->winMode, 0);
                }
            }
            else // windowed
            {
            }

            /* keep the cursor restrained after alt-tabbing */
            if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
            {
                if (rememberFPS != -1)
                {
                    TargetFPS = rememberFPS;
                    DrawFPS = rememberDrawFPS;
                }

                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
                this->render.invalidate = TRUE;
                InterlockedExchange(&this->dd->focusGained, true);
            }
            else if (wParam == WA_INACTIVE)
            {
                rememberFPS = TargetFPS;
                rememberDrawFPS = DrawFPS;
                TargetFPS = 10.0;
                DrawFPS = 0;
                mouse_unlock(false);
            }

            wParam = WA_ACTIVE;
            break;

        case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *pos = (WINDOWPOS *)lParam;
            if ((this->dwFlags & DDSCL_FULLSCREEN) && fsActive && IsWine()
                && (pos->x > 1 || pos->y > 1))
            {
                SetTimer(this->hWnd, TIMER_FIX_WINDOWPOS, 500, (TIMERPROC)NULL);
            }
            break;
        }
        case WM_TIMER:
            switch(wParam)
            {
            case TIMER_FIX_WINDOWPOS:
                KillTimer(this->hWnd, TIMER_FIX_WINDOWPOS);
                SetWindowPos(ddraw->hWnd, HWND_TOP, 1, 1, this->screenWidth, this->screenHeight, SWP_SHOWWINDOW);
                SetWindowPos(ddraw->hWnd, HWND_TOP, 0, 0, this->screenWidth, this->screenHeight, SWP_SHOWWINDOW);
                return 0;
                break;

            case TIMER_EDGE:
                KillTimer(this->hWnd, TIMER_EDGE);
                mouse_unlock(false);
                return 0;
                break;
            default: break;
            }
            break;

        case WM_SIZE:
            switch (wParam)
            {
            case SIZE_MAXIMIZED:
            case SIZE_MAXSHOW:
            case SIZE_RESTORED:
                center_mouse(hWnd);
            default: break;
            }
        case WM_MOVE:
            {
                POINT p = { 0, 0 };
                ClientToScreen(this->dd->hWnd, &p);
                GetClientRect(this->dd->hWnd, &this->winRect);
                OffsetRect(&this->winRect, p.x, p.y);
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
                break;
            }

        /* don't ever tell they lose focus for real so they keep drawing
           Used for windowed mode but also fixes YR menus on alt+tab */
        case WM_ACTIVATEAPP:
            wParam = TRUE;
            lParam = 0;
            break;

        /* make windowed games close on X */
        case WM_SYSCOMMAND:
            if (wParam == SC_CLOSE && GameHandlesClose != true)
            {
                exit(0);
            }
            break;

        case WM_KEYDOWN:

            if(wParam == VK_CONTROL || wParam == VK_TAB)
            {
                if(GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState(VK_TAB) & 0x8000)
                {
                    mouse_unlock(false);
                    return 0;
                }
            }
            if ((GetAsyncKeyState(VK_RMENU) & 0x8000) && GetAsyncKeyState(VK_RCONTROL) & 0x8000)
            {
                if (CaptureMouse)
                {
                    mouse_unlock(true);
                    CaptureMouse = false;
                }
                else
                {
                    mouse_lock(this);
                    CaptureMouse = true;
                }
                return 0;
            }
            if ((wParam == 0x52) && (GetAsyncKeyState(VK_RCONTROL) & 0x8000))
                DrawFPS = !DrawFPS;

            if ((wParam == VK_PRIOR) && (GetAsyncKeyState(VK_RCONTROL) & 0x8000))
            {
                TargetFPS = TargetFPS + 20.0;
                TargetFrameLen = 1000.0 / TargetFPS;
            }

            if ((wParam == VK_NEXT) && (GetAsyncKeyState(VK_RCONTROL) & 0x8000))
            {
                TargetFPS = TargetFPS > 20.0 ? TargetFPS - 20.0 : TargetFPS;
                TargetFrameLen = 1000.0 / TargetFPS;
            }

            if ((wParam == VK_END) && (GetAsyncKeyState(VK_RCONTROL) & 0x8000) && AutoRenderer == true)
            {
        case WM_SWITCHRENDERER:
                if (InterlockedCompareExchange(&Renderer, RENDERER_GDI, RENDERER_OPENGL) == RENDERER_GDI)
                    InterlockedExchange(&Renderer, RENDERER_OPENGL);

                SendMessage(this->dd->hWnd, WM_ACTIVATE, WA_INACTIVE, 0);
                Sleep(50);
                ShowWindow(this->dd->hWnd, SW_RESTORE);
                SendMessage(this->dd->hWnd, WM_ACTIVATE, WA_ACTIVE, 0);
            }
            break;

        case WM_MOUSELEAVE:
        case WM_CLOSE:
            mouse_unlock(false);
            return 0;
            break;

        case WM_MOUSEMOVE:
        {
            LONG xPos = (lParam & 0xffff);
            LONG yPos = (lParam & 0xffff0000) >> 16;
            if (this->edgeTimeoutMs > 0)
            {
                switch (this->edgeDimension)
                {
                case EDGE_X:
                    if (xPos != this->edgeValue)
                    {
                        if (xPos == 0 || xPos == this->width -1)
                        {
                            this->edgeValue = xPos;
                            SetTimer(this->hWnd, TIMER_EDGE, this->edgeTimeoutMs, (TIMERPROC)NULL);
                        }
                        else goto killEdgeTimer;
                    }
                    break;

                case EDGE_Y:
                    if (yPos != this->edgeValue)
                    {
                        if (yPos == 0 || yPos == this->height -1)
                        {
                            this->edgeValue = yPos;
                            SetTimer(this->hWnd, TIMER_EDGE, this->edgeTimeoutMs, (TIMERPROC)NULL);
                        }
                        else goto killEdgeTimer;
                    }
                    break;

                case EDGE_NULL:
                    if (xPos == 0 || xPos == this->width -1)
                    {
                        this->edgeDimension = EDGE_X;
                        this->edgeValue = xPos;
                        SetTimer(this->hWnd, TIMER_EDGE, this->edgeTimeoutMs, (TIMERPROC)NULL);
                    }
                    else if (yPos == 0 || yPos == this->height -1)
                    {
                        this->edgeDimension = EDGE_Y;
                        this->edgeValue = yPos;
                        SetTimer(this->hWnd, TIMER_EDGE, this->edgeTimeoutMs, (TIMERPROC)NULL);
                    }
                    break;
                default: goto killEdgeTimer;
                }
            }
            break;
        killEdgeTimer:
            this->edgeDimension = EDGE_NULL;
            KillTimer(this->hWnd, TIMER_EDGE);
        }
    }

    return this->wndProc(hWnd, uMsg, wParam, lParam);
}

static HRESULT __stdcall _SetCooperativeLevel(IDirectDrawImpl *this, HWND hWnd, DWORD dwFlags)
{
    ENTER;
    dprintf("--> IDirectDraw::SetCooperativeLevel(this=%p, hWnd=%08X, dwFlags=%08X)\n", this, (int)hWnd, (int)dwFlags);
    HRESULT ret = DD_OK;

    if (PROXY)
    {
        ret = IDirectDraw_SetCooperativeLevel(this->real, hWnd, dwFlags);
    }
    else
    {
        /* Windows XP bug */
        if (hWnd == NULL)
        {
            ret = DDERR_INVALIDPARAMS;
        }
        else
        {
            this->bpp = 16;
            this->dwFlags = dwFlags;
            this->hWnd = hWnd;
            this->hDC = GetDC(this->hWnd);
            this->wndProc = (LRESULT(CALLBACK *)(HWND, UINT, WPARAM, LPARAM))GetWindowLong(this->hWnd, GWL_WNDPROC);

            this->edgeTimeoutMs = MonitorEdgeTimer;

            SetWindowLong(this->hWnd, GWL_WNDPROC, (LONG)WndProc);

            if (!IsWindowsXp())
            {
                this->pfd.nSize = sizeof(this->pfd);
                this->pfd.nVersion = 1;

                if (InterlockedExchangeAdd(&Renderer, 0) == RENDERER_OPENGL)
                    this->pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_SWAP_EXCHANGE;
                else
                    this->pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
                this->pfd.iPixelType = PFD_TYPE_RGBA;
                this->pfd.cColorBits = this->bpp;
                this->pfd.iLayerType = PFD_MAIN_PLANE;
                if (!SetPixelFormat(this->hDC, ChoosePixelFormat(this->hDC, &this->pfd), &this->pfd))
                {
                    dprintf("SetPixelFormat failed!\n");
                    ret = DDERR_UNSUPPORTED;
                }
            }

            if (this->screenWidth == 0)
            {
                this->screenWidth = GetSystemMetrics (SM_CXSCREEN);
                this->screenHeight = GetSystemMetrics (SM_CYSCREEN);
            }

            if (this->winMode.dmSize == 0)
            {
                this->winMode.dmSize = sizeof(DEVMODE);
                this->winMode.dmDriverExtra = 0;

                if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &this->winMode) == FALSE)
                {
                    return DDERR_INVALIDMODE;
                }

                this->screenWidth = this->winMode.dmPelsWidth;
                this->screenHeight = this->winMode.dmPelsHeight;
            }

            if (!(this->dwFlags & DDSCL_FULLSCREEN))
            {
                GetClientRect(this->hWnd, &this->winRect);

                this->width = this->winRect.right;
                this->height = this->winRect.bottom;

                if (this->screenWidth > 0 && this->winRect.right > 0)
                {
                    int x = (this->screenWidth / 2) - (this->winRect.right / 2);
                    int y = (this->screenHeight / 2) - (this->winRect.bottom / 2);
                    RECT dst = { x, y, this->winRect.right+x, this->winRect.bottom+y };
                    AdjustWindowRect(&dst, GetWindowLong(this->hWnd, GWL_STYLE), FALSE);
                    SetWindowPos(this->hWnd, HWND_TOP, dst.left, dst.top, (dst.right - dst.left), (dst.bottom - dst.top), SWP_SHOWWINDOW);
                }
            }

            POINT p = { 0, 0 };
            ClientToScreen(this->hWnd, &p);
            GetClientRect(this->hWnd, &this->winRect);
            OffsetRect(&this->winRect, p.x, p.y);
        }
    }

    dprintf("    screen = %dx%d\n", this->screenWidth, this->screenHeight);
    dprintf("<-- IDirectDraw::SetCooperativeLevel(this=%p, hWnd=%08X, dwFlags=%08X) -> %08X\n", this, (int)hWnd, (int)dwFlags, (int)ret);
    LEAVE;
    return ret;
}

static HRESULT __stdcall _WaitForVerticalBlank(IDirectDrawImpl *this, DWORD dwFlags, HANDLE hEvent)
{
    dprintf("--> IDirectDraw::WaitForVerticalBlank(this=%p, dwFlags=%08X, hEvent=%08X)\n", this, (int)dwFlags, (int)hEvent);
    HRESULT ret = IDirectDraw_WaitForVerticalBlank(this->real, dwFlags, hEvent);
    dprintf("<-- IDirectDraw::WaitForVerticalBlank(this=%p, dwFlags=%08X, hEvent=%08X) -> %08X\n", this, (int)dwFlags, (int)hEvent, (int)ret);
    return ret;
}

static IDirectDrawImplVtbl Vtbl =
{
    /* IUnknown */
    _QueryInterface,
    _AddRef,
    _Release,
    /* IDirectDrawImpl */
    _Compact,
    _CreateClipper,
    _CreatePalette,
    _CreateSurface,
    _DuplicateSurface,
    _EnumDisplayModes,
    _EnumSurfaces,
    _FlipToGDISurface,
    _GetCaps,
    _GetDisplayMode,
    _GetFourCCCodes,
    _GetGDISurface,
    _GetMonitorFrequency,
    _GetScanLine,
    _GetVerticalBlankStatus,
    _Initialize,
    _RestoreDisplayMode,
    _SetCooperativeLevel,
    _SetDisplayMode,
    _WaitForVerticalBlank
};
