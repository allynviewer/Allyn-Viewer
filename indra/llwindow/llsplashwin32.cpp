/**
 * @file llsplashwin32.cpp
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */
#include "linden_common.h"
#include "llwindowwin32.h"
#include "lldir.h"
#include "llerror.h"

#include <objbase.h>
#include <wincodec.h>
#include <process.h>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")

struct AllynSplashState
{
	AllynSplashState()
	:	window(NULL)
	,	ready(NULL)
	,	logoPx(0)
	,	winW(0)
	,	winH(0)
	,	barX(0)
	,	barY(0)
	,	barW(0)
	,	barH(0)
	,	glow(0)
	,	stop(0)
	,	progressMilli(80)
	,	startTick(0)
	{
		origin.x = 0;
		origin.y = 0;
	}
	~AllynSplashState()
	{
		if (ready)
		{
			CloseHandle(ready);
			ready = NULL;
		}
	}
	HWND window;
	HANDLE ready;
	std::vector<unsigned char> logo;
	int logoPx;
	int winW;
	int winH;
	int barX;
	int barY;
	int barW;
	int barH;
	int glow;
	POINT origin;
	volatile LONG stop;
	volatile LONG progressMilli;
	ULONGLONG startTick;
};

namespace
{
template<typename T>
struct ComPtr
{
	T* ptr;
	ComPtr() : ptr(NULL) {}
	~ComPtr() { if (ptr) ptr->Release(); }
	T** slot() { return &ptr; }
};

static std::wstring utf8ToWide(const std::string& in)
{
	std::wstring out;
	if (in.empty())
	{
		return out;
	}
	int n = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), NULL, 0);
	if (n <= 0)
	{
		return out;
	}
	out.resize(n);
	MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), &out[0], n);
	return out;
}

static bool pathExists(const std::string& path)
{
	return gDirUtilp && !path.empty() && gDirUtilp->fileExists(path);
}

static std::string splashLogoPath()
{
	if (!gDirUtilp)
	{
		return std::string();
	}
	std::string path = gDirUtilp->getExpandedFilename(LL_PATH_DEFAULT_SKIN, "textures", "allyn_login_logo.png");
	if (pathExists(path))
	{
		return path;
	}
	path = gDirUtilp->findSkinnedFilename(LLDir::TEXTURES, "allyn_login_logo.png", LLDir::ALL_SKINS);
	if (pathExists(path))
	{
		return path;
	}
	const std::string sep = gDirUtilp->getDirDelimiter();
	std::string dir = gDirUtilp->getExecutableDir();
	for (int i = 0; i < 6 && !dir.empty(); ++i)
	{
		std::string beside = dir + sep + "skins" + sep + "default" + sep + "textures" + sep + "allyn_login_logo.png";
		if (pathExists(beside))
		{
			return beside;
		}
		std::string nested = dir + sep + "indra" + sep + "newview" + sep + "skins" + sep + "default" + sep + "textures" + sep + "allyn_login_logo.png";
		if (pathExists(nested))
		{
			return nested;
		}
		size_t slash = dir.find_last_of("\\/");
		if (slash == std::string::npos)
		{
			break;
		}
		dir.erase(slash);
	}
	return std::string();
}

static bool isSplashBackground(const unsigned char* p)
{
	int b = p[0];
	int g = p[1];
	int r = p[2];
	int a = p[3];
	if (a < 8)
	{
		return true;
	}
	int mx = r > g ? r : g;
	if (b > mx) mx = b;
	int mn = r < g ? r : g;
	if (b < mn) mn = b;
	return mx <= 18 && (mx - mn) <= 8;
}

static void punchBackground(std::vector<unsigned char>& px, int w, int h)
{
	const int n = w * h;
	if (n <= 0 || (int)px.size() < n * 4)
	{
		return;
	}
	std::vector<char> bg(n, 0);
	std::vector<int> queue;
	queue.reserve((size_t)n / 8);
	for (int x = 0; x < w; ++x)
	{
		int top = x;
		int bottom = (h - 1) * w + x;
		if (!bg[top] && isSplashBackground(&px[top * 4]))
		{
			bg[top] = 1;
			queue.push_back(top);
		}
		if (bottom != top && !bg[bottom] && isSplashBackground(&px[bottom * 4]))
		{
			bg[bottom] = 1;
			queue.push_back(bottom);
		}
	}
	for (int y = 0; y < h; ++y)
	{
		int left = y * w;
		int right = left + w - 1;
		if (!bg[left] && isSplashBackground(&px[left * 4]))
		{
			bg[left] = 1;
			queue.push_back(left);
		}
		if (right != left && !bg[right] && isSplashBackground(&px[right * 4]))
		{
			bg[right] = 1;
			queue.push_back(right);
		}
	}
	for (size_t qi = 0; qi < queue.size(); ++qi)
	{
		int i = queue[qi];
		int x = i % w;
		int y = i / w;
		int next[4];
		int count = 0;
		if (x > 0) next[count++] = i - 1;
		if (x + 1 < w) next[count++] = i + 1;
		if (y > 0) next[count++] = i - w;
		if (y + 1 < h) next[count++] = i + w;
		for (int k = 0; k < count; ++k)
		{
			int ni = next[k];
			if (bg[ni]) continue;
			if (!isSplashBackground(&px[ni * 4])) continue;
			bg[ni] = 1;
			queue.push_back(ni);
		}
	}
	for (int i = 0; i < n; ++i)
	{
		if (bg[i])
		{
			px[i * 4 + 3] = 0;
		}
	}
	for (int y = 1; y < h - 1; ++y)
	{
		for (int x = 1; x < w - 1; ++x)
		{
			int i = y * w + x;
			unsigned char* p = &px[i * 4];
			if (p[3] < 8) continue;
			if (!(bg[i - 1] || bg[i + 1] || bg[i - w] || bg[i + w])) continue;
			int r = p[2];
			int g = p[1];
			int b = p[0];
			int mx = r > g ? r : g;
			if (b > mx) mx = b;
			int mn = r < g ? r : g;
			if (b < mn) mn = b;
			if (mx < 36 && (mx - mn) < 14)
			{
				p[3] = 0;
			}
		}
	}
	for (int i = 0; i < n; ++i)
	{
		unsigned char* p = &px[i * 4];
		unsigned int a = p[3];
		p[0] = (unsigned char)(p[0] * a / 255);
		p[1] = (unsigned char)(p[1] * a / 255);
		p[2] = (unsigned char)(p[2] * a / 255);
	}
}

static bool decodeLogo(const std::wstring& path, std::vector<unsigned char>& pixels, int target)
{
	if (path.empty() || target <= 0)
	{
		return false;
	}
	HRESULT comHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	bool comOwned = (comHr == S_OK);
	bool ok = false;
	{
		ComPtr<IWICImagingFactory> factory;
		ComPtr<IWICBitmapDecoder> decoder;
		ComPtr<IWICBitmapFrameDecode> frame;
		ComPtr<IWICBitmapScaler> scaler;
		ComPtr<IWICFormatConverter> converter;
		HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.slot()));
		if (SUCCEEDED(hr))
		{
			hr = factory.ptr->CreateDecoderFromFilename(path.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.slot());
		}
		if (SUCCEEDED(hr))
		{
			hr = decoder.ptr->GetFrame(0, frame.slot());
		}
		if (SUCCEEDED(hr))
		{
			hr = factory.ptr->CreateBitmapScaler(scaler.slot());
		}
		if (SUCCEEDED(hr))
		{
			hr = scaler.ptr->Initialize(frame.ptr, (UINT)target, (UINT)target, WICBitmapInterpolationModeFant);
		}
		if (SUCCEEDED(hr))
		{
			hr = factory.ptr->CreateFormatConverter(converter.slot());
		}
		if (SUCCEEDED(hr))
		{
			hr = converter.ptr->Initialize(scaler.ptr, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
		}
		if (SUCCEEDED(hr))
		{
			pixels.resize((size_t)target * (size_t)target * 4);
			hr = converter.ptr->CopyPixels(NULL, (UINT)target * 4, (UINT)pixels.size(), &pixels[0]);
			if (SUCCEEDED(hr))
			{
				punchBackground(pixels, target, target);
				ok = true;
			}
		}
		if (!ok)
		{
			pixels.clear();
		}
	}
	if (comOwned)
	{
		CoUninitialize();
	}
	return ok;
}

static int scaled(int value, int dpi)
{
	return MulDiv(value, dpi, 96);
}

static void blendOver(unsigned char* dst, int r, int g, int b, int a)
{
	if (a <= 0) return;
	if (a > 255) a = 255;
	int inv = 255 - a;
	dst[0] = (unsigned char)((b * a + dst[0] * inv) / 255);
	dst[1] = (unsigned char)((g * a + dst[1] * inv) / 255);
	dst[2] = (unsigned char)((r * a + dst[2] * inv) / 255);
	dst[3] = (unsigned char)((a * 255 + dst[3] * inv) / 255);
}

static float capsuleDist(float px, float py, float x0, float x1, float cy, float rad)
{
	float cx = px;
	if (cx < x0) cx = x0;
	if (cx > x1) cx = x1;
	float dx = px - cx;
	float dy = py - cy;
	return std::sqrt(dx * dx + dy * dy) - rad;
}

static float coverage(float dist)
{
	if (dist <= -1.f) return 1.f;
	if (dist >= 1.f) return 0.f;
	return 0.5f - dist * 0.5f;
}

static void barColor(float u, int& r, int& g, int& b)
{
	if (u < 0.f) u = 0.f;
	if (u > 1.f) u = 1.f;
	if (u < 0.55f)
	{
		float t = u / 0.55f;
		r = (int)(0.f + t * 176.f);
		g = (int)(216.f + t * (64.f - 216.f));
		b = 255;
	}
	else
	{
		float t = (u - 0.55f) / 0.45f;
		r = (int)(176.f + t * (255.f - 176.f));
		g = (int)(64.f + t * (140.f - 64.f));
		b = (int)(255.f + t * (48.f - 255.f));
	}
}

static void drawBar(unsigned char* bits, const AllynSplashState& st, float progress, float shimmer)
{
	if (st.barW <= 0 || st.barH <= 0 || st.winW <= 0)
	{
		return;
	}
	int left = st.barX - st.glow;
	int top = st.barY - st.glow;
	int right = st.barX + st.barW + st.glow;
	int bottom = st.barY + st.barH + st.glow;
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right > st.winW) right = st.winW;
	if (bottom > st.winH) bottom = st.winH;
	float rad = (float)st.barH * 0.5f;
	if (rad < 1.f) rad = 1.f;
	float cy = (float)st.barY + rad;
	float track0 = (float)st.barX + rad;
	float track1 = (float)(st.barX + st.barW) - rad;
	if (track1 < track0) track1 = track0;
	float end = (float)st.barX + (float)st.barW * progress;
	float fill1 = end - rad;
	if (fill1 < track0) fill1 = track0;
	bool hasFill = progress > 0.004f && end > (float)st.barX + 1.f;
	float glowRad = rad + (float)st.glow * 0.72f;
	for (int y = top; y < bottom; ++y)
	{
		unsigned char* row = bits + (size_t)y * (size_t)st.winW * 4;
		for (int x = left; x < right; ++x)
		{
			float px = (float)x + 0.5f;
			float py = (float)y + 0.5f;
			unsigned char* dst = row + (size_t)x * 4;
			if (hasFill)
			{
				float glowCov = coverage(capsuleDist(px, py, track0, fill1, cy, glowRad));
				if (glowCov > 0.f)
				{
					float u = (px - (float)st.barX) / (float)st.barW;
					int cr, cg, cb;
					barColor(u, cr, cg, cb);
					blendOver(dst, cr, cg, cb, (int)(glowCov * 64.f));
				}
			}
			float dTrack = capsuleDist(px, py, track0, track1, cy, rad);
			float trackCov = coverage(dTrack);
			if (trackCov > 0.f)
			{
				blendOver(dst, 255, 255, 255, (int)(trackCov * 150.f));
				float rim = 1.8f - std::fabs(dTrack);
				if (rim < 0.f) rim = 0.f;
				if (rim > 1.f) rim = 1.f;
				blendOver(dst, 90, 220, 255, (int)(rim * 230.f));
			}
			if (!hasFill)
			{
				continue;
			}
			float fillCov = coverage(capsuleDist(px, py, track0, fill1, cy, rad));
			if (fillCov <= 0.f)
			{
				continue;
			}
			float u = (px - (float)st.barX) / (float)st.barW;
			int cr, cg, cb;
			barColor(u, cr, cg, cb);
			float ny = (py - cy) / rad;
			if (ny < 0.f) ny = -ny;
			if (ny > 1.f) ny = 1.f;
			int lift = (int)((1.f - ny) * 42.f);
			cr += lift; if (cr > 255) cr = 255;
			cg += lift; if (cg > 255) cg = 255;
			cb += lift; if (cb > 255) cb = 255;
			blendOver(dst, cr, cg, cb, (int)(fillCov * 240.f));
			float du = u - shimmer;
			if (du > 0.5f) du -= 1.f;
			if (du < -0.5f) du += 1.f;
			float shine = std::exp(-du * du * 80.f);
			blendOver(dst, 255, 255, 255, (int)(shine * fillCov * 130.f));
		}
	}
}
}

LLSplashScreenWin32::LLSplashScreenWin32()
:	mWindow(NULL)
,	mThread(NULL)
,	mThreadId(0)
,	mState(NULL)
{
}

LLSplashScreenWin32::~LLSplashScreenWin32()
{
	hideImpl();
}

void LLSplashScreenWin32::loadLogo()
{
	if (!mState)
	{
		return;
	}
	HDC screen = GetDC(NULL);
	int dpi = 96;
	if (screen)
	{
		dpi = GetDeviceCaps(screen, LOGPIXELSX);
		ReleaseDC(NULL, screen);
	}
	if (dpi < 96)
	{
		dpi = 96;
	}
	int logo = scaled(330, dpi);
	int gap = scaled(18, dpi);
	int barW = scaled(280, dpi);
	int barH = scaled(14, dpi);
	int glow = scaled(14, dpi);
	if (barH < 6) barH = 6;
	if (glow < 4) glow = 4;
	if (barW > logo - 8) barW = logo - 8;
	int bottom = glow + scaled(4, dpi);
	mState->logoPx = logo;
	mState->glow = glow;
	mState->barW = barW;
	mState->barH = barH;
	mState->barX = (logo - barW) / 2;
	mState->barY = logo + gap;
	mState->winW = logo;
	mState->winH = logo + gap + barH + bottom;
	std::wstring wide = utf8ToWide(splashLogoPath());
	if (!decodeLogo(wide, mState->logo, logo))
	{
		LL_WARNS("Window") << "Splash logo was not loaded" << LL_ENDL;
		mState->logo.clear();
		mState->logoPx = 0;
		mState->barX = glow;
		mState->barY = glow;
		mState->winW = barW + glow * 2;
		mState->winH = barH + glow * 2 + scaled(4, dpi);
	}
	POINT cursor;
	cursor.x = 0;
	cursor.y = 0;
	GetCursorPos(&cursor);
	HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);
	int x = 0;
	int y = 0;
	if (GetMonitorInfo(monitor, &info))
	{
		x = info.rcWork.left + ((info.rcWork.right - info.rcWork.left) - mState->winW) / 2;
		y = info.rcWork.top + ((info.rcWork.bottom - info.rcWork.top) - mState->winH) / 2;
	}
	mState->origin.x = x;
	mState->origin.y = y;
	mState->startTick = GetTickCount64();
}

void LLSplashScreenWin32::presentFrame()
{
	if (!mState || !mState->window || mState->winW <= 0 || mState->winH <= 0)
	{
		return;
	}
	AllynSplashState& st = *mState;
	float seconds = (float)(GetTickCount64() - st.startTick) / 1000.f;
	if (seconds < 0.f) seconds = 0.f;
	float creep = (1.f - std::exp(-seconds / 12.f)) * 0.93f;
	float staged = (float)InterlockedCompareExchange(&st.progressMilli, 0, 0) / 1000.f;
	float progress = creep > staged ? creep : staged;
	if (progress > 0.93f) progress = 0.93f;
	float shimmer = seconds * 0.72f;
	shimmer -= std::floor(shimmer);

	HDC screen = GetDC(NULL);
	if (!screen)
	{
		return;
	}
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = st.winW;
	bmi.bmiHeader.biHeight = -st.winH;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* bits = NULL;
	HBITMAP dib = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
	if (!dib || !bits)
	{
		if (dib) DeleteObject(dib);
		ReleaseDC(NULL, screen);
		return;
	}
	memset(bits, 0, (size_t)st.winW * (size_t)st.winH * 4);
	if (!st.logo.empty() && st.logoPx > 0)
	{
		int rows = st.logoPx;
		if (rows > st.winH) rows = st.winH;
		int cols = st.logoPx;
		if (cols > st.winW) cols = st.winW;
		for (int y = 0; y < rows; ++y)
		{
			memcpy((unsigned char*)bits + (size_t)y * (size_t)st.winW * 4,
				&st.logo[(size_t)y * (size_t)st.logoPx * 4],
				(size_t)cols * 4);
		}
	}
	drawBar((unsigned char*)bits, st, progress, shimmer);
	HDC mem = CreateCompatibleDC(screen);
	HGDIOBJ old = SelectObject(mem, dib);
	POINT src;
	src.x = 0;
	src.y = 0;
	SIZE size;
	size.cx = st.winW;
	size.cy = st.winH;
	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = AC_SRC_ALPHA;
	BOOL presented = UpdateLayeredWindow(st.window, screen, &st.origin, &size, mem, &src, 0, &blend, ULW_ALPHA);
	if (!presented)
	{
		static volatile LONG sLoggedFail = 0;
		if (InterlockedCompareExchange(&sLoggedFail, 1, 0) == 0)
		{
			LL_WARNS("Window") << "UpdateLayeredWindow failed: " << GetLastError() << LL_ENDL;
		}
	}
	SelectObject(mem, old);
	DeleteObject(dib);
	DeleteDC(mem);
	ReleaseDC(NULL, screen);
}

void LLSplashScreenWin32::runSplash()
{
	MSG kick;
	PeekMessage(&kick, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	if (!mState || mState->winW <= 0 || mState->winH <= 0)
	{
		if (mState && mState->ready) SetEvent(mState->ready);
		return;
	}
	HINSTANCE inst = GetModuleHandle(NULL);
	WNDCLASSEXW wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = DefWindowProcW;
	wc.hInstance = inst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"AllynViewerSplash";
	RegisterClassExW(&wc);
	mState->window = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		L"AllynViewerSplash",
		L"Allyn Viewer",
		WS_POPUP,
		mState->origin.x,
		mState->origin.y,
		mState->winW,
		mState->winH,
		NULL,
		NULL,
		inst,
		NULL);
	if (mState->window)
	{
		presentFrame();
		ShowWindow(mState->window, SW_SHOWNOACTIVATE);
	}
	else
	{
		LL_WARNS("Window") << "Splash window was not created: " << GetLastError() << LL_ENDL;
	}
	if (mState->ready)
	{
		SetEvent(mState->ready);
	}
	if (!mState->window)
	{
		return;
	}
	while (InterlockedCompareExchange(&mState->stop, 0, 0) == 0)
	{
		presentFrame();
		MsgWaitForMultipleObjects(0, NULL, FALSE, 33, QS_ALLINPUT);
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	if (mState->window)
	{
		DestroyWindow(mState->window);
		mState->window = NULL;
	}
}

unsigned __stdcall LLSplashScreenWin32::animThread(void* param)
{
	LLSplashScreenWin32* self = (LLSplashScreenWin32*)param;
	if (self)
	{
		self->runSplash();
	}
	return 0;
}

void LLSplashScreenWin32::showImpl()
{
	mState = new AllynSplashState();
	mState->ready = CreateEvent(NULL, TRUE, FALSE, NULL);
	loadLogo();
	if (!mState->ready)
	{
		delete mState;
		mState = NULL;
		HINSTANCE hinst = GetModuleHandle(NULL);
		mWindow = CreateDialog(hinst,
			TEXT("SPLASHSCREEN"),
			NULL,
			(DLGPROC)LLSplashScreenWin32::windowProc);
		if (mWindow)
		{
			ShowWindow(mWindow, SW_SHOW);
		}
		return;
	}
	uintptr_t handle = _beginthreadex(
		NULL,
		0,
		reinterpret_cast<unsigned (__stdcall *)(void*)>(&LLSplashScreenWin32::animThread),
		this,
		0,
		&mThreadId);
	mThread = (HANDLE)handle;
	if (mThread)
	{
		WaitForSingleObject(mState->ready, 8000);
	}
	if (!mState->window)
	{
		if (mState)
		{
			InterlockedExchange(&mState->stop, 1);
		}
		if (mThreadId)
		{
			PostThreadMessage(mThreadId, WM_NULL, 0, 0);
		}
		if (mThread)
		{
			WaitForSingleObject(mThread, 2000);
			CloseHandle(mThread);
			mThread = NULL;
			mThreadId = 0;
		}
		delete mState;
		mState = NULL;
		HINSTANCE hinst = GetModuleHandle(NULL);
		mWindow = CreateDialog(hinst,
			TEXT("SPLASHSCREEN"),
			NULL,
			(DLGPROC)LLSplashScreenWin32::windowProc);
		if (mWindow)
		{
			ShowWindow(mWindow, SW_SHOW);
		}
	}
}

void LLSplashScreenWin32::updateImpl(const std::string& mesg)
{
	if (mThread && mState)
	{
		LONG cur = InterlockedCompareExchange(&mState->progressMilli, 0, 0);
		LONG next = cur + 90;
		if (next > 900) next = 900;
		InterlockedExchange(&mState->progressMilli, next);
		return;
	}
	if (!mWindow) return;
	int output_str_len = MultiByteToWideChar(CP_UTF8, 0, mesg.c_str(), mesg.length(), NULL, 0);
	if (output_str_len > 1024)
	{
		return;
	}
	WCHAR w_mesg[1025];
	MultiByteToWideChar(CP_UTF8, 0, mesg.c_str(), mesg.length(), w_mesg, output_str_len);
	w_mesg[output_str_len] = 0;
	SendDlgItemMessage(mWindow, 666, WM_SETTEXT, FALSE, (LPARAM)w_mesg);
}

void LLSplashScreenWin32::hideImpl()
{
	if (mState)
	{
		InterlockedExchange(&mState->stop, 1);
	}
	if (mThreadId)
	{
		PostThreadMessage(mThreadId, WM_NULL, 0, 0);
	}
	if (mThread)
	{
		WaitForSingleObject(mThread, 5000);
		CloseHandle(mThread);
		mThread = NULL;
		mThreadId = 0;
	}
	if (mWindow)
	{
		DestroyWindow(mWindow);
		mWindow = NULL;
	}
	delete mState;
	mState = NULL;
}

LRESULT CALLBACK LLSplashScreenWin32::windowProc(HWND h_wnd, UINT u_msg,
											WPARAM w_param, LPARAM l_param)
{
	return DefWindowProc(h_wnd, u_msg, w_param, l_param);
}
