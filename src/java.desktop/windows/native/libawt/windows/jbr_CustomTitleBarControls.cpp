// Copyright 2000-2023 JetBrains s.r.o.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//
// Created by nikita.gubarkov on 26.01.2023.
//

#define GDIPVER 0x0110
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;

#include "jbr_CustomTitleBarControls.h"

namespace CustomTitleBarControlsSupport {
    CriticalSection criticalSection;

    using State = CustomTitleBarControls::State;
    using Type = CustomTitleBarControls::Type;
    using ButtonColors = ARGB[2][(int)State::UNKNOWN]; // [Background/Foreground][State]
    static const ARGB BC_INHERIT = 0x00ffffff; // Transparent white means inherit

    ButtonColors DEFAULT_COLORS_WIN11[3] = { // Light/Dark/Close
            //  NORMAL  // HOVERED  // PRESSED  // DISABLED // INACTIVE //
            {{BC_INHERIT, 0x0A000000, 0x06000000, BC_INHERIT, BC_INHERIT},  // Light background
             {0xFF000000, 0xFF000000, 0xFF000000, 0x33000000, 0x60000000}}, // Light foreground
            {{BC_INHERIT, 0x0FFFFFFF, 0x0BFEFEFE, BC_INHERIT, BC_INHERIT},  // Dark  background
             {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x33FFFFFF, 0x60FFFFFF}}, // Dark  foreground
            {{BC_INHERIT, 0xFFC42B1C, 0xE5C32B1B, BC_INHERIT, BC_INHERIT},  // Close background
             {BC_INHERIT, 0xFFFFFFFF, 0xFFFFFFFF, BC_INHERIT, BC_INHERIT}}, // Close foreground
    };

    ButtonColors DEFAULT_COLORS_WIN10[3] = { // Light/Dark/Close
            //  NORMAL  // HOVERED  // PRESSED  // DISABLED // INACTIVE //
            {{BC_INHERIT, 0x1A000000, 0x33000000, BC_INHERIT, BC_INHERIT},  // Light background
             {0xFF000000, 0xFF000000, 0xFF000000, 0x33000000, 0x60000000}}, // Light foreground
            {{BC_INHERIT, 0x1AFEFEFE, 0x33FFFFFF, BC_INHERIT, BC_INHERIT},  // Dark  background
             {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x33FFFFFF, 0x60FFFFFF}}, // Dark  foreground
            {{BC_INHERIT, 0xFFE81123, 0x99E71022, BC_INHERIT, BC_INHERIT},  // Close background
             {BC_INHERIT, 0xFFFFFFFF, 0xFFFFFFFF, BC_INHERIT, BC_INHERIT}}, // Close foreground
    };

    void PaintIconWin11(Type type, Graphics& g, float scale, SolidBrush* mask) {
        float size = 10.0 * scale;
        GraphicsPath p {};
        switch (type) {
            case Type::CLOSE:{
                float o = 0.3f;
                Pen pen(mask, 1.04 * scale);
                p.AddLine(o, o, size-o, size-o);
                p.CloseFigure();
                p.AddLine(size-o, o, o, size-o);
                g.DrawPath(&pen, &p);
                if (scale < 1.5f) {
                    g.SetCompositingMode(CompositingModeSourceOver);
                    g.DrawPath(&pen, &p);
                }
            } return;
            case Type::MINIMIZE:{
                float t = (int) (4.0 * scale);
                if (scale > 2 && ((int) (2.0 * scale)) % 2 == 1) t += 0.5;
                p.AddArc(0.0, t, scale, scale, 90, 180);
                p.AddArc(size-scale, t, scale, scale, 270, 180);
            } break;
            case Type::RESTORE:{
                float r = 6.0 * scale, d = 3.0 * scale, o = 2.0 * scale;
                float a = 19.4712206f; // asin(1/3) in degrees
                p.AddArc(o, 0.0, d, d, 180+a, 90-a);
                p.AddArc(size-r, 0.0, r, r, 270, 90);
                p.AddArc(size-d, size-d-o, d, d, 0, 90-a);
                d = 4.0 * scale;
                p.AddArc(size-(r+d)/2.0, (r-d)/2.0, d, d, 0, -90);
                p.CloseFigure();
            }{
                size = (int) (8.0 * scale);
                float r = 3.0 * scale, d = 1.0 * scale, t = (r-d)/2.0, o = (r+d)/2.0, y = 10.0 * scale - size;
                p.AddArc(0.0, y, r, r, 180, 90);
                p.AddArc(size-r, y, r, r, 270, 90);
                p.AddArc(size-r, size-r+y, r, r, 0, 90);
                p.AddArc(0.0, size-r+y, r, r, 90, 90);
                p.CloseFigure();
                p.AddArc(t, t+y, d, d, 180, 90);
                p.AddArc(size-o, t+y, d, d, 270, 90);
                p.AddArc(size-o, size-o+y, d, d, 0, 90);
                p.AddArc(t, size-o+y, d, d, 90, 90);
                p.CloseFigure();
            } break;
            case Type::MAXIMIZE:{
                float r = 3.0 * scale, d = 1.0 * scale, t = (r-d)/2.0, o = (r+d)/2.0;
                p.AddArc(0.0, 0.0, r, r, 180, 90);
                p.AddArc(size-r, 0.0, r, r, 270, 90);
                p.AddArc(size-r, size-r, r, r, 0, 90);
                p.AddArc(0.0, size-r, r, r, 90, 90);
                p.CloseFigure();
                p.AddArc(t, t, d, d, 180, 90);
                p.AddArc(size-o, t, d, d, 270, 90);
                p.AddArc(size-o, size-o, d, d, 0, 90);
                p.AddArc(t, size-o, d, d, 90, 90);
                p.CloseFigure();
            } break;
        }
        g.FillPath(mask, &p);
    }

    void PaintIconWin10(Type type, Graphics& g, float scale, SolidBrush* mask) {
        SolidBrush clear(0xff000000);
        g.SetSmoothingMode(SmoothingModeNone);
        float size = 10.0 * scale;
        switch (type) {
            case Type::CLOSE:{
                float o = scale * 0.35;
                Pen pen(mask, scale);
                g.DrawLine(&pen, o, o, size-o, size-o);
                g.DrawLine(&pen, size-o, o, o, size-o);
            } break;
            case Type::MINIMIZE:{
                float t = (int) (4.0 * scale);
                g.FillRectangle(mask, 0.0, t, size, scale);
            } break;
            case Type::RESTORE:{
                float r = (int) (8.0 * scale), t = (int) scale;
                g.FillRectangle(mask, size-r, 0.0, r, r);
                g.FillRectangle(&clear, size-r+t, t, r-t*2.0, r-t*2.0);
                g.FillRectangle(mask, 0.0, size-r, r, r);
                g.FillRectangle(&clear, t, size-r+t, r-t*2.0, r-t*2.0);
            } break;
            case Type::MAXIMIZE:{
                float t = (int) scale;
                g.FillRectangle(mask, 0.0, 0.0, size, size);
                g.FillRectangle(&clear, t, t, size-t*2.0, size-t*2.0);
            } break;
        }
    }

    void (*PaintIcon)(Type type, Graphics& g, float scale, SolidBrush* mask);
    ButtonColors* DEFAULT_COLORS;

    Color GetColor(Type type, State state, BOOL foreground, BOOL dark, const ButtonColors& override) {
        if (type == Type::CLOSE) {
            ARGB result = DEFAULT_COLORS[2][(int)foreground][(int)state];
            if (result != BC_INHERIT) return result;
        }
        ARGB result = override[(int)foreground][(int)state];
        if (result != BC_INHERIT) return result;
        return DEFAULT_COLORS[(int)dark][(int)foreground][(int)state];
    }

    Bitmap* CreateIcon(Type type, float scale) {
        int size = (int) (10.0f * scale + 0.5f); // All icons are 10x10px at 100% scale
        int stride = ((size * 3 + 3) / 4) * 4;
        BYTE* bitmapData = new BYTE[size * stride];
        Bitmap* bitmap = new Bitmap(size, size, stride, PixelFormat24bppRGB, bitmapData);
        SolidBrush mask(0xffffffff);
        Graphics g(bitmap);
        g.SetCompositingMode(CompositingModeSourceCopy);
        g.SetSmoothingMode(SmoothingModeAntiAlias8x8);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        g.Clear(0xff000000);
        PaintIcon(type, g, scale, &mask);
        return bitmap;
    }

    static const int ICON_SCALES = 7;
    static const float ICON_SCALE_MAP[ICON_SCALES][2] = {
            {1.0f, 1.0f},
            {1.25f, 1.2f},
            {1.5f, 1.5f},
            {2.0f, 2.0f},
            {2.5f, 2.4f},
            {3.0f, 3.0f},
            {4.0f, 4.0f},
    };
    Bitmap* ICON_CACHE[(int) Type::UNKNOWN][ICON_SCALES] {};

    Bitmap* GetIcon(Type type, float scale, int scaleId) {
        CriticalSection::Lock lock(criticalSection);
        Bitmap*& cached = ICON_CACHE[(int) type][scaleId];
        if (!cached) cached = CreateIcon(type, scale);
        return cached;
    }

    Bitmap* GetIcon(Type type, float scale) {
        int i = ICON_SCALES-1;
        while (i > 0) {
            if (scale >= ICON_SCALE_MAP[i][0]) break;
            else i--;
        }
        scale = ICON_SCALE_MAP[i][1];
        return GetIcon(type, scale, i);
    }

    enum class Availability {
        UNKNOWN,
        AVAILABLE,
        UNAVAILABLE
    };
    volatile Availability availability = Availability::UNKNOWN;
    BOOL (*ShouldSystemUseDarkMode)() = NULL;

    BOOL IsAvailable() {
        if (availability != Availability::UNKNOWN) {
            return availability == Availability::AVAILABLE;
        }
        CriticalSection::Lock lock(criticalSection);
        if (availability != Availability::UNKNOWN) {
            return availability == Availability::AVAILABLE;
        }

        ULONG_PTR startupToken;
        GdiplusStartupInput input;
        if (GdiplusStartup(&startupToken, &input, NULL) != Ok) {
            availability = Availability::UNAVAILABLE;
            return FALSE;
        }

        JNIEnv *env = (JNIEnv *) JNU_GetEnv(jvm, JNI_VERSION_1_2);
        BOOL win11OrNewer = JNU_GetStaticFieldByName(env, NULL, "sun/awt/windows/WFramePeer", "WIN11_OR_NEWER", "Z").z;
        if (win11OrNewer) {
            PaintIcon = PaintIconWin11;
            DEFAULT_COLORS = DEFAULT_COLORS_WIN11;
        } else {
            PaintIcon = PaintIconWin10;
            DEFAULT_COLORS = DEFAULT_COLORS_WIN10;
        }

        availability = Availability::AVAILABLE;
        return TRUE;
    }
}
using namespace CustomTitleBarControlsSupport;
extern BOOL AppsUseLightThemeCached;

// === CustomTitleBarControls implementation ===

class CustomTitleBarControls::Resources {
public:

    const SIZE size;
    HDC hdc;
    BYTE* bitmapData;
    Bitmap* bitmap;
    HBITMAP hbitmap;
    Graphics* graphics;

    Resources(SIZE s, HDC hdcComp) :
        size(s) {
        bitmapData = new BYTE[s.cx * s.cy * 4];
        bitmap = new Bitmap(s.cx, s.cy, s.cx * 4, PixelFormat32bppPARGB, bitmapData);
        bitmap->GetHBITMAP(Color(0), &hbitmap);
        hdc = CreateCompatibleDC(hdcComp);
        SelectObject(hdc, hbitmap);
        graphics = Graphics::FromHDC(hdc);
    }

    ~Resources() {
        delete graphics;
        DeleteDC(hdc);
        DeleteObject(hbitmap);
        delete bitmap;
        delete bitmapData;
    }
};

class CustomTitleBarControls::Style {

    static jobject GetProperty(JNIEnv* env, jobject properties, LPCWSTR key) {
        jstring jkey = JNU_NewStringPlatform(env, key);
        jobject value = JNU_CallMethodByName(env, NULL, properties, "get", "(Ljava/lang/Object;)Ljava/lang/Object;", jkey).l;
        env->DeleteLocalRef(jkey);
        return value;
    }

    static void UnwrapProperty(JNIEnv* env, jobject properties, LPCWSTR key, const char* instanceof,
                               const char* unwrapMethod, const char* unwrapSignature, jvalue& result) {
        jobject value = GetProperty(env, properties, key);
        if (value) {
            if (JNU_IsInstanceOfByName(env, value, instanceof) == 1) {
                result = JNU_CallMethodByName(env, NULL, value, unwrapMethod, unwrapSignature);
            }
            env->DeleteLocalRef(value);
        }
    }

    static int GetBooleanProperty(JNIEnv* env, jobject properties, LPCWSTR key) { // null -> -1
        jvalue result;
        result.i = -1;
        UnwrapProperty(env, properties, key, "java/lang/Boolean", "booleanValue", "()Z", result);
        return (int) result.i;
    }

    static float GetNumberProperty(JNIEnv* env, jobject properties, LPCWSTR key) { // null -> -1
        jvalue result;
        result.f = -1;
        UnwrapProperty(env, properties, key, "java/lang/Number", "floatValue", "()F", result);
        return (float) result.f;
    }

    static ARGB GetColorProperty(JNIEnv* env, jobject properties, LPCWSTR key) { // null -> BC_INHERIT
        jvalue result;
        result.i = BC_INHERIT;
        UnwrapProperty(env, properties, key, "java/awt/Color", "getRGB", "()I", result);
        return (ARGB) result.i;
    }

public:
    float height, width;
    int dark;
    ButtonColors colors;

    BOOL Update(jobject target, JNIEnv* env) {
        jobject titleBar = JNU_GetFieldByName(env, NULL, target, "customTitleBar", "Ljava/awt/Window$CustomTitleBar;").l;
        if (!titleBar) return FALSE;
        jobject properties = JNU_CallMethodByName(env, NULL, titleBar, "getProperties", "()Ljava/util/Map;").l;
        BOOL visible = TRUE;
        if (properties) {
            if (GetBooleanProperty(env, properties, L"controls.visible") == 0) {
                visible = FALSE;
            } else {
                height = JNU_CallMethodByName(env, NULL, titleBar, "getHeight", "()F").f;
                width = GetNumberProperty(env, properties, L"controls.width");
                dark = GetBooleanProperty(env, properties, L"controls.dark");
                // Get colors
                #define GET_STATE_COLOR(NAME, PROPERTY) \
                colors[0][(int)State::NAME] = GetColorProperty(env, properties, L"controls.background." PROPERTY); \
                colors[1][(int)State::NAME] = GetColorProperty(env, properties, L"controls.foreground." PROPERTY)
                GET_STATE_COLOR(NORMAL, L"normal");
                GET_STATE_COLOR(HOVERED, L"hovered");
                GET_STATE_COLOR(PRESSED, L"pressed");
                GET_STATE_COLOR(DISABLED, L"disabled");
                GET_STATE_COLOR(INACTIVE, L"inactive");
            }
            env->DeleteLocalRef(properties);
        }
        env->DeleteLocalRef(titleBar);
        return visible;
    }
};

CustomTitleBarControls* CustomTitleBarControls::CreateIfNeeded(HWND parent, jobject target, JNIEnv* env) {
    Style style;
    if (IsAvailable() && style.Update(target, env)) {
        return new CustomTitleBarControls(parent, style);
    } else {
        return NULL;
    }
}

CustomTitleBarControls::CustomTitleBarControls(HWND parent, const Style& style) {
    this->parent = parent;

    LPCTSTR CLASS = L"JBRCustomTitleBarControls";
    WNDCLASSEX wc;
    if (!GetClassInfoEx(AwtToolkit::GetInstance().GetModuleHandle(), CLASS, &wc)) {
        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.style         = 0L;
        wc.lpfnWndProc   = (WNDPROC)DefWindowProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = AwtToolkit::GetInstance().GetModuleHandle(),
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = CLASS;
        wc.hIconSm       = NULL;
        RegisterClassEx(&wc);
    }

    hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, CLASS, L"",
                          WS_CHILD | WS_VISIBLE,
                          0, 0, 0, 0,
                          parent, 0, AwtToolkit::GetInstance().GetModuleHandle(), 0);

    hit = HTNOWHERE;
    pressed = FALSE;
    windowState = State::NORMAL;
    resources = NULL;
    this->style = new Style(style);
    Update();
}

CustomTitleBarControls::~CustomTitleBarControls() {
    DestroyWindow(hwnd);
    delete resources;
    delete style;
}

BOOL CustomTitleBarControls::UpdateStyle(jobject target, JNIEnv* env) {
    if (style->Update(target, env)) {
        Update();
        return TRUE;
    }
    return FALSE;
}

void CustomTitleBarControls::PaintButton(Type type, State state, int x, int width, float scale, BOOL dark) {
    Graphics& g = *resources->graphics;

    // Paint background
    Color background = GetColor(type, state, false, dark, style->colors);
    if (background.GetA() > 0) {
        SolidBrush brush(background);
        g.FillRectangle(&brush, x, 0, width, resources->size.cy);
    }

    // Paint icon
    Color foreground = GetColor(type, state, true, dark, style->colors);
    float c[4] = {(float)foreground.GetA() / 255.0f,
                  (float)foreground.GetR() / 255.0f,
                  (float)foreground.GetG() / 255.0f,
                  (float)foreground.GetB() / 255.0f};
    ColorMatrix colorMatrix = {
            0.0f, 0.0f, 0.0f, c[0], 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            c[1], c[2], c[3], 0.0f, 1.0f};
    Bitmap* i = GetIcon(type, scale);
    int w = i->GetWidth(), h = i->GetHeight();
    ImageAttributes  imageAttributes;
    imageAttributes.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
    g.DrawImage(i, Rect(x + (width - w) / 2, (resources->size.cy - h) / 2, w, h), 0, 0, w, h, UnitPixel, &imageAttributes);
}

#define LOAD_STYLE_BITS()                                        \
DWORD styleBits = (DWORD) GetWindowLong(parent, GWL_STYLE);      \
DWORD exStyleBits = (DWORD) GetWindowLong(parent, GWL_EXSTYLE);  \
BOOL allButtons = styleBits & (WS_MINIMIZEBOX | WS_MAXIMIZEBOX); \
BOOL ltr = !(exStyleBits & WS_EX_LAYOUTRTL)

void CustomTitleBarControls::Update(State windowState) {
    LOAD_STYLE_BITS();

    // Calculate size
    float userWidth;
    if (style->width > 0.0f) {
        userWidth = style->width;
    } else {
        userWidth = allButtons ? 141 : 32;
    }
    UINT dpi = AwtToolkit::GetDpiForWindow(hwnd);
    float scale = (float) dpi / 96.0f;
    SIZE newSize {(int) (userWidth * scale), (int) (style->height * scale)};

    // Recreate resources if size has changed
    if (!resources || resources->size.cx != newSize.cx || resources->size.cy != newSize.cy) {
        delete resources;
        HDC hdcComp = GetDC(hwnd);
        resources = new Resources(newSize, hdcComp);
        ReleaseDC(hwnd, hdcComp);
    }

    // Calculate states
    if (windowState != State::UNKNOWN) this->windowState = windowState;
    State minState = this->windowState, maxState = minState, closeState = minState;
    if (hit != HTNOWHERE) {
        State& s = hit == HTMINBUTTON ? minState : hit == HTMAXBUTTON ? maxState : closeState;
        s = pressed ? State::PRESSED : State::HOVERED;
    }
    if (!(styleBits & WS_MINIMIZEBOX)) minState = State::DISABLED;
    if (!(styleBits & WS_MAXIMIZEBOX)) maxState = State::DISABLED;

    BOOL dark = style->dark != -1 ? (BOOL) style->dark : !AppsUseLightThemeCached;

    // Paint buttons
    resources->graphics->Clear(0);
    if (allButtons) {
        int w = newSize.cx / 3;
        Type maxType = IsZoomed(parent) ? Type::RESTORE : Type::MAXIMIZE;
        if (ltr) {
            PaintButton(Type::MINIMIZE, minState, 0, w, scale, dark);
            PaintButton(maxType, maxState, w, w, scale, dark);
            PaintButton(Type::CLOSE, closeState, w*2, newSize.cx-w*2, scale, dark);
        } else {
            PaintButton(Type::CLOSE, closeState, 0, newSize.cx-w*2, scale, dark);
            PaintButton(maxType, maxState, newSize.cx-w*2, w, scale, dark);
            PaintButton(Type::MINIMIZE, minState, newSize.cx-w, w, scale, dark);
        }
    } else {
        PaintButton(Type::CLOSE, closeState, 0, newSize.cx, scale, dark);
    }

    // Update window
    POINT position {0, 0}, ptSrc {0, 0};
    if (ltr) {
        RECT parentRect;
        GetClientRect(parent, &parentRect);
        position.x = parentRect.right - newSize.cx;
    }

    BLENDFUNCTION blend;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;

    HDC hdcDst = GetDC(NULL);
    SetWindowPos(hwnd, HWND_TOP, position.x, position.y, newSize.cx, newSize.cy, 0);
    UpdateLayeredWindow(hwnd, hdcDst, &position, &newSize, resources->hdc, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(NULL, hdcDst);
}

LRESULT CustomTitleBarControls::Hit(HitType type, int ncx, int ncy) {
    LRESULT newHit = HTNOWHERE;
    if (type != HitType::RESET) {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        if (ncx >= rect.left && ncx <= rect.right && ncy >= rect.top && ncy <= rect.bottom) {
            LOAD_STYLE_BITS();
            newHit = HTCLOSE;
            if (allButtons) {
                int w = (rect.right - rect.left) / 3;
                ncx -= rect.left;
                if (!ltr) ncx = rect.right - rect.left - ncx;
                if (ncx < w) newHit = HTMINBUTTON;
                else if (ncx < w*2) newHit = HTMAXBUTTON;
            }
        }
    }
    if (type == HitType::TEST) return newHit;
    if (newHit != hit || type == HitType::PRESS || type == HitType::RELEASE) {
        LRESULT oldHit = hit;
        hit = newHit;
        if (type == HitType::PRESS) pressed = true;
        else if (type == HitType::RELEASE || newHit != oldHit) {
            if (!pressed && type == HitType::RELEASE) newHit = HTNOWHERE; // Cancel action
            pressed = false;
        }
        Update();

        TRACKMOUSEEVENT track;
        track.cbSize = sizeof(TRACKMOUSEEVENT);
        track.dwFlags = TME_LEAVE | TME_NONCLIENT;
        track.hwndTrack = parent;
        TrackMouseEvent(&track);
    }
    return newHit;
}
