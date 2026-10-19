// SPDX-License-Identifier: GPL-3.0-or-later
package de.maggu2810.android.stroopy.qt;

import android.view.KeyEvent;

import org.qtproject.qt.android.bindings.QtActivity;

/**
 * Qt drops gamepad buttons (A/B/X/Y, shoulders, Start/Select) instead of turning them into key
 * events, so they are forwarded to the native Gamepad object here. The D-pad still reaches Qt
 * as arrow keys.
 */
public class StroopyActivity extends QtActivity {
    private static native void onGamepadButton(int keyCode, boolean pressed);

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (KeyEvent.isGamepadButton(event.getKeyCode())) {
            if (event.getRepeatCount() == 0) {
                try {
                    onGamepadButton(event.getKeyCode(), event.getAction() == KeyEvent.ACTION_DOWN);
                } catch (UnsatisfiedLinkError e) {
                    // Native side not loaded yet.
                }
            }
            return true;
        }
        return super.dispatchKeyEvent(event);
    }
}
