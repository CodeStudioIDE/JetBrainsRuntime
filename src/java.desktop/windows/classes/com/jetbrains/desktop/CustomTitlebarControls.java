/*
 * Copyright 2000-2023 JetBrains s.r.o.
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

package com.jetbrains.desktop;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.geom.AffineTransform;
import java.awt.image.BufferedImage;
import java.io.Serial;
import java.io.Serializable;
import java.util.Collections;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Consumer;

class CustomTitlebarControls implements Serializable {
    @Serial
    private static final long serialVersionUID = -8125606075697848352L;

    private static Component create(Window window, float height, Map<String, Object> params,
                                    MouseAdapter minCallback, MouseAdapter maxCallback, MouseAdapter closeCallback) {
        if (!(window instanceof Frame) && !(window instanceof Dialog)) return null;
        CustomTitlebarControls controls = new CustomTitlebarControls(window, height, params,
                minCallback, maxCallback, closeCallback);
        return controls.panel;
    }

    private enum Type {
        MINIMIZE,
        MAXIMIZE,
        RESTORE,
        CLOSE
    }

    private enum State {
        NORMAL,
        HOVERED, // "Hot" in Windows theme terminology
        PRESSED, // "Pushed" in Windows theme terminology
        DISABLED,
        INACTIVE; // Didn't find this state in Windows, it represents button in inactive window

        private static final State[] VALUES = values();
    }

    private final Boolean dark; // null means default
    private final EnumMap<State, Color> foreground, background;
    private EnumMap<State, Color> defaultForeground, defaultBackground;

    private final Window window;
    private final Container panel;
    private final IButton min, max, close;

    private CustomTitlebarControls(Window window, float height, Map<String, Object> params,
                                   MouseAdapter minCallback, MouseAdapter maxCallback, MouseAdapter closeCallback) {

        dark = params.get("dark") instanceof Boolean b ? b : null;
        foreground = new EnumMap<>(State.class);
        background = new EnumMap<>(State.class);
        for (State state : State.VALUES) {
            String name = state.name().toLowerCase();
            Object f = params.get("foreground." + name);
            Object b = params.get("background." + name);
            if (f instanceof Color c) foreground.put(state, c);
            if (b instanceof Color c) background.put(state, c);
        }

        this.window = window;
        boolean lightweight = window instanceof JFrame || window instanceof JDialog;
        boolean frame = window instanceof Frame;
        int width = frame ? 141 : 34; // Default width
        if (get.apply("width") instanceof Number n && n.intValue() > 0) width = n.intValue();
        panel = lightweight ? new LPanel() : new HPanel();
        panel.setLayout(null);
        panel.setBackground(null);
        panel.setPreferredSize(new Dimension(width, Math.round(height)));
        if (frame) {
            min = lightweight ? new LButton() : new HButton();
            min.setType(Type.MINIMIZE);
            min.component().addMouseListener(minCallback);
            min.component().addMouseMotionListener(minCallback);
            panel.add(min.component());
            max = lightweight ? new LButton() : new HButton();
            max.setType(Type.MAXIMIZE);
            max.component().addMouseListener(maxCallback);
            max.component().addMouseMotionListener(maxCallback);
            panel.add(max.component());
        } else {
            min = max = null;
        }
        close = lightweight ? new LButton() : new HButton();
        close.setType(Type.CLOSE);
        close.component().addMouseListener(closeCallback);
        close.component().addMouseMotionListener(closeCallback);
        panel.add(close.component());
    }

    private void layout() {
        Dimension d = panel.getSize();
        if (min != null && max != null) {
            int w = d.width / 3;
            if (window.getComponentOrientation().isLeftToRight()) {
                min.component().setBounds(0, 0, w, d.height);
                max.component().setBounds(w, 0, w, d.height);
                close.component().setBounds(w*2, 0, d.width - w*2, d.height);
            } else {
                close.component().setBounds(0, 0, d.width - w*2, d.height);
                max.component().setBounds(d.width - w*2, 0, w, d.height);
                min.component().setBounds(d.width - w, 0, w, d.height);
            }
        } else {
            close.component().setBounds(0, 0, d.width, d.height);
        }
    }

    private float alignment() {
        return window.getComponentOrientation().isLeftToRight() ? 1.0f : 0.0f;
    }

    private void paintPanel(Container panel, Graphics g) {
        boolean dark;
        if (this.dark != null) dark = this.dark;
        else {
            // Try guessing theme by background luminance.
            Color c = panel.getBackground();
            if (c == null) dark = false;
            else dark = (0.2126*c.getRed() + 0.7152*c.getGreen() + 0.0722*c.getBlue()) < 128.0;
        }
        defaultForeground = dark ? DARK_FOREGROUND : LIGHT_FOREGROUND;
        defaultBackground = dark ? DARK_BACKGROUND : LIGHT_BACKGROUND;
        if (max != null && window instanceof Frame frame) {
            max.setType((frame.getExtendedState() & Frame.MAXIMIZED_BOTH) != 0 ? Type.RESTORE : Type.MAXIMIZE);
            max.component().setEnabled(frame.isResizable());
        }

        Color color = getColor(null, State.NORMAL, false);
        if (color != null) {
            g.setColor(color);
            g.fillRect(0, 0, panel.getWidth(), panel.getHeight());
        }
    }

    private Color getColor(Type type, State state, boolean foreground) {
        if (type == Type.CLOSE) {
            Color c = (foreground ? CLOSE_FOREGROUND : CLOSE_BACKGROUND).get(state);
            if (c != null) return c;
        }
        Color c = (foreground ? this.foreground : this.background).get(state);
        if (c != null) return c;
        return (foreground ? this.defaultForeground : this.defaultBackground).get(state);
    }

    private void paintButton(IButton button, Graphics g) {
        State state = button.getState();
        Type type = button.getType();

        // Draw background
        Color background = getColor(type, state, false);
        if (background != null) {
            g.setColor(background);
            g.fillRect(0, 0, button.component().getWidth(), button.component().getHeight());
        }

        // Draw icon
        g.translate(button.component().getWidth() / 2, button.component().getHeight() / 2);
        double scale = 1.0;
        if (g instanceof Graphics2D g2d) {
            AffineTransform transform = g2d.getTransform();
            scale = (Math.abs(transform.getScaleX()) + Math.abs(transform.getScaleY())) / 2.0;
            g2d.scale(1.0 / scale, 1.0 / scale);
        }
        BufferedImage icon = getIcon(type, getColor(type, state, true), (float) scale);
        if (icon != null) g.drawImage(icon, -icon.getWidth() / 2, -icon.getHeight() / 2, null);
    }

    // Default colors
    private static final EnumMap<State, Color>
            LIGHT_BACKGROUND, LIGHT_FOREGROUND,
            DARK_BACKGROUND, DARK_FOREGROUND,
            CLOSE_BACKGROUND, CLOSE_FOREGROUND;
    private static final EnumMap<Type, Consumer<Graphics2D>> ICON_PAINTERS = new EnumMap<>(Type.class);
    private static final Color MASK_ON = Color.WHITE, MASK_OFF = new Color(0, true);
    private record IconDescriptor(Type type, Color color, int scale) {}
    private static final Map<IconDescriptor, BufferedImage> ICON_CACHE = Collections.synchronizedMap(new HashMap<>());

    static {
        // 0 means "inherit" ->    |  NORMAL  || HOVERED  || PRESSED  || DISABLED || INACTIVE |
        LIGHT_BACKGROUND = colorMap(0,          0x0A000000, 0x06000000, 0,          0         );
        LIGHT_FOREGROUND = colorMap(0xFF000000, 0xFF000000, 0xFF000000, 0x33000000, 0x60000000);
        DARK_BACKGROUND  = colorMap(0,          0x0FFFFFFF, 0x0BFEFEFE, 0,          0         );
        DARK_FOREGROUND  = colorMap(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x33FFFFFF, 0x60FFFFFF);
        CLOSE_BACKGROUND = colorMap(0,          0xFFC42B1C, 0xE5C32B1B, 0,          0         );
        CLOSE_FOREGROUND = colorMap(0         , 0xFFFFFFFF, 0xFFFFFFFF, 0         , 0         );

        boolean win11AndNewer;
        if ("Windows 10".equals(System.getProperty("os.name"))) {
            win11AndNewer = false;
        } else {
            try {
                int version = (int) Double.parseDouble(System.getProperty("os.version"));
                win11AndNewer = version >= 10;
            } catch (NullPointerException | NumberFormatException e) {
                win11AndNewer = true;
            }
        }
        if (win11AndNewer) { // Windows 11 style
            ICON_PAINTERS.put(Type.MINIMIZE, g -> {
                g.setColor(MASK_ON);
                g.fillRoundRect(0, 4, 10, 1, 1, 1);
            });
            ICON_PAINTERS.put(Type.MAXIMIZE, g -> {
                g.setColor(MASK_ON);
                g.fillRoundRect(0, 0, 10, 10, 3, 3);
                g.setColor(MASK_OFF);
                g.fillRoundRect(1, 1, 8, 8, 1, 1);
            });
            ICON_PAINTERS.put(Type.RESTORE, g -> {
                g.setColor(MASK_ON);
                g.fillRoundRect(2, 0, 8, 8, 6, 6);
                g.fillRoundRect(2, 0, 4, 4, 3, 3);
                g.fillRoundRect(6, 4, 4, 4, 3, 3);
                g.setColor(MASK_OFF);
                g.fillRoundRect(0, 1, 9, 9, 4, 4);
                g.setColor(MASK_ON);
                g.fillRoundRect(0, 2, 8, 8, 3, 3);
                g.setColor(MASK_OFF);
                g.fillRoundRect(1, 3, 6, 6, 1, 1);
            });
            ICON_PAINTERS.put(Type.CLOSE, g -> {
                // We need more subpixel precision here, so we have precise control of thickness and its scale.
                // Resulting shape thickness (px) = t / s
                final int t = 11, s = 16; // This value was carefully tuned to look like native icons.
                // Helper variables: 0t nhx mf
                final int h = s * 5, n = h - t, x = h + t, f = s * 10, m = f - t;
                int[][] points = {
                        {0, n, 0}, // x
                        {t, h, m}, // y
                        {t, h, m}, // x
                        {f, x, f}, // y
                };
                g.setColor(MASK_ON);
                g.fillRoundRect(0, 0, 10, 10, 1, 1);
                g.scale(1.0f / (float) s, 1.0f / (float) s);
                g.setColor(MASK_OFF);
                g.fillPolygon(points[0], points[1], 3);
                g.fillPolygon(points[1], points[0], 3);
                g.fillPolygon(points[2], points[3], 3);
                g.fillPolygon(points[3], points[2], 3);
            });
        } else { // Windows 10 has different background colors
            // 0 means "inherit" ->         |  NORMAL  || HOVERED  || PRESSED  || DISABLED || INACTIVE |
            LIGHT_BACKGROUND.putAll(colorMap(0,          0x1A000000, 0x33000000, 0,          0         ));
            DARK_BACKGROUND.putAll( colorMap(0,          0x1AFEFEFE, 0x33FFFFFF, 0,          0         ));
            CLOSE_BACKGROUND.putAll(colorMap(0,          0xFFE81123, 0x99E71022, 0,          0         ));
            ICON_PAINTERS.put(Type.MINIMIZE, g -> {
                g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
                g.setColor(MASK_ON);
                g.fillRect(0, 4, 10, 1);
            });
            ICON_PAINTERS.put(Type.MAXIMIZE, g -> {
                g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
                g.setColor(MASK_ON);
                g.fillRect(0, 0, 10, 10);
                g.setColor(MASK_OFF);
                g.fillRect(1, 1, 8, 8);
            });
            ICON_PAINTERS.put(Type.RESTORE, g -> {
                g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
                g.setColor(MASK_ON);
                g.fillRect(2, 0, 8, 8);
                g.setColor(MASK_OFF);
                g.fillRect(3, 1, 6, 6);
                g.setColor(MASK_ON);
                g.fillRect(0, 2, 8, 8);
                g.setColor(MASK_OFF);
                g.fillRect(1, 3, 6, 6);
            });
            ICON_PAINTERS.put(Type.CLOSE, g -> {
                g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_OFF);
                // We need more subpixel precision here, so we have precise control of thickness and its scale.
                // Resulting shape thickness (px) = t / s
                final int t = 11, s = 16; // This value was carefully tuned to look like native icons.
                // Helper variables: 0t nhx mf
                final int h = s * 5, n = h - t, x = h + t, f = s * 10, m = f - t;
                int[][] points = {
                        {0, n, 0, t, h, m, f, x, f, m, h, t}, // x
                        {t, h, m, f, x, f, m, h, t, 0, n, 0}, // y
                };
                g.setColor(MASK_ON);
                double noAntialiasingCorrectionOffset = -1.0 / (g.getTransform().getScaleX() + g.getTransform().getScaleY());
                g.translate(noAntialiasingCorrectionOffset, noAntialiasingCorrectionOffset);
                g.scale(1.0f / (float) s, 1.0f / (float) s);
                g.fillPolygon(points[0], points[1], 12);
            });
        }
    }

    private static BufferedImage getIcon(Type type, Color color, float scale) {
        if (type == null || color == null) return null;
        // This is how Windows quantize icon sizes - 100%, 125%, then each 50%
        int s = switch (Math.round(scale * 4)) {
            case 4 -> 0; // 100%
            case 5 -> 1; // 125%
            default -> (int) Math.floor(scale * 2) - 1; // 150%...
        };
        // Clamp
        if (s < 0) s = 0;
        else if (s > 31) s = 31; // 16x scale
        // Recalculate scale
        scale = switch (s) {
            case 0 -> 1.0f;
            case 1 -> 1.2f;
            default -> ((float) s / 2.0f) + 0.5f;
        };
        // Get icon from cache or draw new one
        return getIcon(new IconDescriptor(type, color, s), scale);
    }

    private static BufferedImage getIcon(IconDescriptor descriptor, float scale) {
        BufferedImage icon = ICON_CACHE.get(descriptor);
        if (icon != null) return icon;
        if (descriptor.color.equals(MASK_ON)) {
            icon = createMaskIcon(descriptor.type, scale);
            ICON_CACHE.put(descriptor, icon);
            return icon;
        }
        icon = createColorIcon(getIcon(
                new IconDescriptor(descriptor.type, MASK_ON, descriptor.scale), scale), descriptor.color);
        ICON_CACHE.put(descriptor, icon);
        return icon;
    }

    private static BufferedImage createMaskIcon(Type type, float scale) {
        int size = Math.round(scale * 10f); // All icons are 10x10px at 100% scale
        BufferedImage mask = new BufferedImage(size, size, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = mask.createGraphics();
        g.setComposite(AlphaComposite.Src);
        g.setColor(MASK_OFF);
        g.fillRect(0, 0, size, size);
        g.scale(scale, scale);
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        ICON_PAINTERS.get(type).accept(g);
        g.dispose();
        return mask;
    }

    private static BufferedImage createColorIcon(BufferedImage mask, Color color) {
        BufferedImage image = new BufferedImage(mask.getWidth(), mask.getHeight(), BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = image.createGraphics();
        g.drawImage(mask, 0, 0, null);
        g.setComposite(AlphaComposite.SrcIn); // Multiply our color by alpha from mask image.
        g.setColor(color);
        g.fillRect(0, 0, mask.getWidth(), mask.getHeight());
        g.dispose();
        return image;
    }

    private static EnumMap<State, Color> colorMap(int... argb) {
        if (argb.length != State.VALUES.length) throw new IllegalArgumentException("Invalid number of states");
        EnumMap<State, Color> map = new EnumMap<>(State.class);
        for (int i = 0; i < argb.length; i++) {
            if (argb[i] != 0) map.put(State.VALUES[i], new Color(argb[i], true));
        }
        return map;
    }

    private class HPanel extends Panel {
        @Serial
        private static final long serialVersionUID = -4001281044233476450L;
        @Override
        public void doLayout() { CustomTitlebarControls.this.layout(); }
        @Override
        public float getAlignmentX() { return CustomTitlebarControls.this.alignment(); }
        @Override
        public void paint(Graphics g) {
            CustomTitlebarControls.this.paintPanel(this, g);
            if (min != null) min.component().repaint();
            if (max != null) max.component().repaint();
            if (close != null) close.component().repaint();
        }
    }

    private class LPanel extends JPanel {
        @Serial
        private static final long serialVersionUID = -606904252949842495L;
        private LPanel() { setOpaque(false); setBorder(null); }
        @Override
        public void doLayout() { CustomTitlebarControls.this.layout(); }
        @Override
        public float getAlignmentX() { return CustomTitlebarControls.this.alignment(); }
        @Override
        public void paintComponent(Graphics g) { CustomTitlebarControls.this.paintPanel(this, g); }
    }

    private interface IButton extends Serializable {
        Component component();
        void setType(Type type);
        Type getType();
        State getState();
    }

    private class HButton extends Canvas implements IButton, MouseListener {
        private Type type;
        private boolean hovered, pressed;
        @Serial
        private static final long serialVersionUID = -3850079412620901803L;
        private HButton() {
            setBackground(null);
            setFocusable(false);
            addMouseListener(this);
        }
        @Override
        public Component component() { return this; }
        @Override
        public void setType(Type type) { this.type = type; }
        @Override
        public Type getType() { return type; }
        @Override
        public State getState() {
            if (!isEnabled()) return State.DISABLED;
            else if (pressed) return State.PRESSED;
            else if (hovered) return State.HOVERED;
            else if (!window.isActive()) return State.INACTIVE;
            else return State.NORMAL;
        }
        @Override
        public void paint(Graphics g) { CustomTitlebarControls.this.paintButton(this, g); }
        @Override
        public void mouseClicked(MouseEvent e) {}
        @Override
        public void mousePressed(MouseEvent e) { pressed = true; repaint(); }
        @Override
        public void mouseReleased(MouseEvent e) { pressed = false; repaint(); }
        @Override
        public void mouseEntered(MouseEvent e) { hovered = true; repaint(); }
        @Override
        public void mouseExited(MouseEvent e) { hovered = pressed = false; repaint(); }
    }

    private class LButton extends JButton implements IButton {
        private Type type;
        @Serial
        private static final long serialVersionUID = -3850079412620901803L;
        private LButton() {
            setBackground(null);
            setFocusable(false);
            setBorder(null);
            setOpaque(false);
            setRolloverEnabled(true);
        }
        @Override
        public Component component() { return this; }
        @Override
        public void setType(Type type) { this.type = type; }
        @Override
        public Type getType() { return type; }
        @Override
        public State getState() {
            ButtonModel model = getModel();
            if (!model.isEnabled()) return State.DISABLED;
            else if (model.isPressed()) return State.PRESSED;
            else if (model.isRollover()) return State.HOVERED;
            else if (!window.isActive()) return State.INACTIVE;
            else return State.NORMAL;
        }
        @Override
        public void paintComponent(Graphics g) { CustomTitlebarControls.this.paintButton(this, g); }
    }
}
