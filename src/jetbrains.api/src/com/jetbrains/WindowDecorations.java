/*
 * Copyright 2000-2022 JetBrains s.r.o.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package com.jetbrains;

import java.awt.*;
import java.util.Map;

/**
 * Window decorations consist of titlebar, window controls and border.
 * @see WindowDecorations.CustomTitlebar
 */
public interface WindowDecorations {

    /**
     * @see CustomTitlebar
     */
    void setCustomTitlebar(Frame frame, CustomTitlebar customTitlebar);

    /**
     * @see CustomTitlebar
     */
    void setCustomTitlebar(Dialog dialog, CustomTitlebar customTitlebar);

    /**
     * You must {@linkplain CustomTitlebar#setHeight(float) set titlebar height} before adding it to a window.
     * @see CustomTitlebar
     */
    CustomTitlebar createCustomTitlebar();

    /**
     * Custom titlebar allows merging of window content with native title bar,
     * which is done by treating title bar as part of client area, but with some
     * special behavior like dragging or maximizing on double click.
     * Custom titlebar has {@linkplain CustomTitlebar#getHeight()  height} and controls.
     * @implNote Behavior is platform-dependent, only macOS and Windows are supported.
     * @see #setCustomTitlebar(Frame, CustomTitlebar)
     */
    interface CustomTitlebar {

        /**
         * @return titlebar height, measured in pixels from the top of client area, i.e. excluding top frame border.
         */
        float getHeight();

        /**
         * @param height titlebar height, measured in pixels from the top of client area,
         *               i.e. excluding top frame border. Must be > 0.
         */
        void setHeight(float height);

        /**
         * @see #putProperty(String, Object)
         */
        Map<String, Object> getProperties();

        /**
         * @see #putProperty(String, Object)
         */
        void putProperties(Map<? extends String, ?> m);

        /**
         * Windows & macOS properties:
         * <ul>
         *     <li>{@code controls.visible} : {@link Boolean} - whether titlebar controls
         *         (minimize/maximize/close buttons) are visible, default = true.</li>
         * </ul>
         * Windows properties:
         * <ul>
         *     <li>{@code controls.width} : {@link Number} - width of block of buttons (not individual buttons).
         *         Note that dialogs have only one button, while frames usually have 3 of them.</li>
         *     <li>{@code controls.dark} : {@link Boolean} - whether to use dark or light color theme
         *         (light or dark icons respectively).</li>
         *     <li>{@code controls.<layer>.<state>} : {@link Color} - precise control over button colors,
         *         where {@code <layer>} is one of:
         *         <ul><li>{@code foreground}</li><li>{@code background}</li></ul>
         *         and {@code <state>} is one of:
         *         <ul>
         *             <li>{@code normal}</li>
         *             <li>{@code hovered}</li>
         *             <li>{@code pressed}</li>
         *             <li>{@code disabled}</li>
         *             <li>{@code inactive}</li>
         *         </ul>
         * </ul>
         */
        void putProperty(String key, Object value);

        /**
         * @return space occupied by titlebar controls on the left (px)
         */
        float getLeftInset();
        /**
         * @return space occupied by titlebar controls on the right (px)
         */
        float getRightInset();

        /**
         * Override current hit test value for titlebar.
         * This can affect native titlebar behavior like dragging and maximizing on double-click.
         * <ul>
         *     <li>{@code client=true} means that mouse is currently over a client area. Native titlebar behavior is disabled.</li>
         *     <li>{@code client=false} means that mouse is currently over a non-client area. Native titlebar behavior is enabled.</li>
         * </ul>
         * <em>Intended usage:
         * <ul>
         *     <li>This method must be called in response to all {@linkplain java.awt.event.MouseEvent mouse events}
         *         except {@link java.awt.event.MouseEvent#MOUSE_EXITED} and {@link java.awt.event.MouseEvent#MOUSE_WHEEL}.</li>
         *     <li>This method is called per-event, i.e. when component has multiple listeners, you only need to call it once.</li>
         *     <li>If this method hadn't been called, titlebar behavior is reverted back to default upon processing the event.</li>
         * </ul></em>
         * Note that hit test value is relevant only for titlebar area, e.g. calling
         * {@code forceHitTest(false)} will not make window draggable via non-titlebar area.
         */
        void forceHitTest(boolean client);

        Window getContainingWindow();
    }
}
