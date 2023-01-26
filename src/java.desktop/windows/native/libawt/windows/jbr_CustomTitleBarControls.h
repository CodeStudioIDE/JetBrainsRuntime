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

#ifndef JBR_CUSTOMTITLEBARCONTROLS_H
#define JBR_CUSTOMTITLEBARCONTROLS_H

#include "awt_Toolkit.h"

namespace Gdiplus {
    class GpBitmap;
    class GpGraphics;
}

class CustomTitleBarControls {
public:

    enum class State {
        NORMAL,
        HOVERED, // "Hot" in Windows theme terminology
        PRESSED, // "Pushed" in Windows theme terminology
        DISABLED,
        INACTIVE, // Didn't find this state in Windows, it represents button in inactive window
        UNKNOWN
    };

    enum class Type {
        MINIMIZE,
        MAXIMIZE,
        RESTORE,
        CLOSE,
        UNKNOWN
    };

    enum class HitType {
        RESET,
        TEST,
        MOVE,
        PRESS,
        RELEASE
    };

private:
    class Resources;
    class Style;

    HWND parent, hwnd;
    Resources* resources;
    Style* style;
    LRESULT hit;
    BOOL pressed;
    State windowState;

    CustomTitleBarControls(HWND parent, const Style& style);
    void PaintButton(Type type, State state, int x, int width, float scale, BOOL dark);

public:
    static CustomTitleBarControls* CreateIfNeeded(HWND parent, jobject target, JNIEnv* env);
    BOOL UpdateStyle(jobject target, JNIEnv* env);
    void Update(State windowState = State::UNKNOWN);
    LRESULT Hit(HitType type, int ncx, int ncy); // HTNOWHERE / HTMINBUTTON / HTMAXBUTTON / HTCLOSE
    ~CustomTitleBarControls();

};

#endif //JBR_CUSTOMTITLEBARCONTROLS_H
