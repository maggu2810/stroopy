// SPDX-License-Identifier: GPL-3.0-or-later
// Service worker: precaches the app so it works offline; the build injects the file list.
/* eslint-disable @typescript-eslint/no-explicit-any */
const sw = self as any;
declare const PRECACHE: string[];
declare const CACHE_NAME: string;

sw.addEventListener("install", (event: any) => {
  event.waitUntil(
    caches
      .open(CACHE_NAME)
      .then((cache) => cache.addAll(PRECACHE))
      .then(() => sw.skipWaiting()),
  );
});

sw.addEventListener("activate", (event: any) => {
  event.waitUntil(
    caches
      .keys()
      .then((keys) => Promise.all(keys.filter((k) => k !== CACHE_NAME).map((k) => caches.delete(k))))
      .then(() => sw.clients.claim()),
  );
});

// Cache first for our own files; the page itself falls back to the cached index.html offline.
sw.addEventListener("fetch", (event: any) => {
  const request: Request = event.request;
  const url = new URL(request.url);
  if (request.method !== "GET" || url.origin !== location.origin) return;
  event.respondWith(
    caches.match(request, { ignoreSearch: request.mode === "navigate" }).then(
      (hit) => hit ?? fetch(request).catch(() => caches.match("./index.html").then((r) => r ?? Response.error())),
    ),
  );
});
