// SPDX-License-Identifier: GPL-3.0-or-later
// Mirrors the page stack in the browser history, so the system back gesture
// (Android, browser back button) acts like the in-app back button.
import { effect } from "@preact/signals";
import { back } from "./input.ts";
import { stack } from "./nav.ts";

/** Depth (stack length - 1) of the history entry we are on. */
let depth = 0;
/** Popstate events caused by our own history.go() calls. */
let ignore = 0;

const depthOf = (state: unknown) => (state as { stroopy?: number } | null)?.stroopy ?? 0;

function sync() {
  // Wait for a pending history.go(); its popstate calls sync() again.
  if (ignore > 0) return;
  const target = stack.value.length - 1;
  if (target > depth) {
    while (depth < target) history.pushState({ stroopy: ++depth }, "");
  } else if (target < depth) {
    ignore++;
    history.go(target - depth);
    depth = target;
  }
}

export function installHistory() {
  history.replaceState({ stroopy: 0 }, "");
  window.addEventListener("popstate", (e) => {
    const reached = depthOf(e.state);
    if (ignore > 0) {
      ignore--;
      sync();
      return;
    }
    if (reached > depth) {
      // Forward: the stack has no pages ahead, so undo it.
      ignore++;
      history.go(depth - reached);
      return;
    }
    depth = reached;
    // The page decides what back means (quit a game, leave a room...); sync()
    // then restores entries if the stack is still deeper than the history.
    back();
    sync();
  });
  effect(sync);
}
