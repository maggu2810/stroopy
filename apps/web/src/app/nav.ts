// SPDX-License-Identifier: GPL-3.0-or-later
// Page stack (screens of docs/design/product.md).
import { signal } from "@preact/signals";

export type Page =
  | { name: "home" }
  | { name: "game" }
  | { name: "result"; rank: number }
  | { name: "highscores" }
  | { name: "enterCode"; text?: string }
  | { name: "settings" }
  | { name: "backup" }
  | { name: "help" };

export const stack = signal<Page[]>([{ name: "home" }]);

export const push = (page: Page) => (stack.value = [...stack.value, page]);
export const pop = () => (stack.value = stack.value.length > 1 ? stack.value.slice(0, -1) : stack.value);
/** Replaces the whole stack: home, then `page` (if any). */
export const reset = (page?: Page) => (stack.value = page ? [{ name: "home" }, page] : [{ name: "home" }]);
