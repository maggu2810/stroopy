// SPDX-License-Identifier: GPL-3.0-or-later
import type { ComponentChildren } from "preact";
import { useEffect } from "preact/hooks";
import { signal } from "@preact/signals";
import { focusFirst, usePageInput, type PageInput } from "../app/input.ts";

/** Secondary page with a title bar and a back button. */
export function SubPage(props: { title: string; onBack: () => void; input?: PageInput; class?: string; children: ComponentChildren }) {
  usePageInput({ back: props.onBack, ...props.input });
  useEffect(() => focusFirst(), []);
  return (
    <div class={`page sub ${props.class ?? ""}`} data-page>
      <header class="bar">
        <button class="icon" tabIndex={-1} aria-label="Back" onClick={props.onBack}>
          ‹
        </button>
        <h2>{props.title}</h2>
      </header>
      <main class="scroll">{props.children}</main>
    </div>
  );
}

const toastText = signal("");
let toastTimer: ReturnType<typeof setTimeout> | undefined;

export function toast(text: string) {
  toastText.value = text;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => (toastText.value = ""), 2200);
}

export function Toast() {
  return toastText.value ? (
    <div class="toast" role="status">
      {toastText.value}
    </div>
  ) : null;
}

export async function copyText(text: string): Promise<boolean> {
  try {
    await navigator.clipboard.writeText(text);
    return true;
  } catch {
    return false;
  }
}

export async function pasteText(): Promise<string | null> {
  try {
    return await navigator.clipboard.readText();
  } catch {
    return null;
  }
}

/** The platform share sheet when there is one (phones), else the clipboard. Returns what happened. */
export async function shareText(text: string): Promise<"shared" | "copied" | "failed"> {
  if (navigator.share && matchMedia("(pointer: coarse)").matches) {
    try {
      await navigator.share({ text });
      return "shared";
    } catch {
      // cancelled: fall through to the clipboard
    }
  }
  return (await copyText(text)) ? "copied" : "failed";
}
