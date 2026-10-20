// SPDX-License-Identifier: GPL-3.0-or-later
// Keyboard, gamepad and focus navigation (docs/design/product.md, "Input").
import { signal } from "@preact/signals";
import { useEffect } from "preact/hooks";

/** Set once a key or gamepad button is used: show key hints and the focus frame. */
export const keyHints = signal(false);

// Standard Gamepad API mapping.
export const Pad = { A: 0, B: 1, X: 2, Y: 3, L1: 4, R1: 5, Select: 8, Start: 9, Up: 12, Down: 13, Left: 14, Right: 15 } as const;

export interface PageInput {
  /** Return true if the key was handled. */
  key?: (e: KeyboardEvent) => boolean;
  pad?: (button: number) => boolean;
  back?: () => void;
}

let current: PageInput = {};
/** Shoulder buttons held (Find: outer colours). */
export let shoulderHeld = false;

/** Registers the active page's handlers while it is mounted. */
export function usePageInput(input: PageInput) {
  useEffect(() => {
    const previous = current;
    current = input;
    return () => {
      current = previous;
    };
  });
}

function isTextInput(el: Element | null): boolean {
  return el instanceof HTMLTextAreaElement || (el instanceof HTMLInputElement && !["checkbox", "radio", "button"].includes(el.type)) || el instanceof HTMLSelectElement;
}

function focusables(): HTMLElement[] {
  const root = document.querySelector("[data-page]:last-of-type") ?? document.body;
  return [...root.querySelectorAll<HTMLElement>("button, input, select, textarea, a[href], [tabindex]:not([tabindex='-1'])")].filter(
    (el) => !el.hasAttribute("disabled") && el.offsetParent !== null && el.tabIndex >= 0,
  );
}

export function moveFocus(forward: boolean) {
  const list = focusables();
  if (!list.length) return;
  const i = list.indexOf(document.activeElement as HTMLElement);
  const next = i < 0 ? list[0]! : list[(i + (forward ? 1 : list.length - 1)) % list.length]!;
  next.focus();
}

/** Focuses the first element matching `selector` on the current page (or the first focusable one). */
export function focusFirst(selector = "[data-autofocus]") {
  requestAnimationFrame(() => {
    const root = document.querySelector("[data-page]:last-of-type");
    const el = root?.querySelector<HTMLElement>(selector) ?? focusables()[0];
    el?.focus({ preventScroll: true });
  });
}

function onKey(e: KeyboardEvent) {
  keyHints.value = true;
  document.documentElement.classList.add("key-hints");
  if (e.key === "F11" || e.altKey || e.ctrlKey || e.metaKey) return;
  if (current.key?.(e)) {
    e.preventDefault();
    return;
  }
  const typing = isTextInput(document.activeElement);
  if (e.key === "Escape") {
    if (current.back) {
      current.back();
      e.preventDefault();
    }
    return;
  }
  if (typing) return;
  if (e.key === "ArrowDown" || e.key === "ArrowRight") moveFocus(true);
  else if (e.key === "ArrowUp" || e.key === "ArrowLeft") moveFocus(false);
  else return;
  e.preventDefault();
}

function onPad(button: number) {
  keyHints.value = true;
  document.documentElement.classList.add("key-hints");
  if (current.pad?.(button)) return;
  switch (button) {
    case Pad.Up:
    case Pad.Left:
      moveFocus(false);
      break;
    case Pad.Down:
    case Pad.Right:
      moveFocus(true);
      break;
    case Pad.A:
      (document.activeElement as HTMLElement | null)?.click();
      break;
    case Pad.B:
      current.back?.();
      break;
  }
}

let previousButtons: boolean[][] = [];

function pollGamepads() {
  const pads = navigator.getGamepads?.() ?? [];
  let shoulder = false;
  pads.forEach((pad, i) => {
    if (!pad) return;
    const pressed = pad.buttons.map((b) => b.pressed);
    shoulder ||= !!(pressed[Pad.L1] || pressed[Pad.R1]);
    const before = previousButtons[i] ?? [];
    shoulderHeld = shoulder;
    pressed.forEach((down, b) => {
      if (down && !before[b] && b !== Pad.L1 && b !== Pad.R1) onPad(b);
    });
    previousButtons[i] = pressed;
  });
  shoulderHeld = shoulder;
  if (pads.some(Boolean)) requestAnimationFrame(pollGamepads);
}

export function installInput() {
  window.addEventListener("keydown", onKey);
  window.addEventListener("pointerdown", () => {
    keyHints.value = false;
    document.documentElement.classList.remove("key-hints");
  });
  window.addEventListener("gamepadconnected", () => requestAnimationFrame(pollGamepads));
}
