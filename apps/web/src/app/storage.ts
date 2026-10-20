// SPDX-License-Identifier: GPL-3.0-or-later
// Browser storage. It can be unavailable (private mode) or evicted, so every access is guarded
// and the backup code is the safety net (docs/decisions/0010-web-app-pwa.md).

const PREFIX = "stroopy.";

export function load<T>(key: string, fallback: T): T {
  try {
    const raw = localStorage.getItem(PREFIX + key);
    return raw === null ? fallback : (JSON.parse(raw) as T);
  } catch {
    return fallback;
  }
}

export function save(key: string, value: unknown): void {
  try {
    localStorage.setItem(PREFIX + key, JSON.stringify(value));
  } catch {
    // storage full or disabled: the in-memory state still works for this session
  }
}

/** Asks the browser not to evict our storage under pressure (installed web apps usually get it). */
export async function requestPersistence(): Promise<void> {
  try {
    if (navigator.storage?.persisted && !(await navigator.storage.persisted())) await navigator.storage.persist();
  } catch {
    // not supported
  }
}
