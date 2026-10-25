// SPDX-License-Identifier: GPL-3.0-or-later
import { render } from "preact";
import * as backup from "./app/backup.ts";
import { controller } from "./app/controller.ts";
import * as highscores from "./app/highscores.ts";
import * as multiplayer from "./app/multiplayer.ts";
import { installHistory } from "./app/history.ts";
import { installInput } from "./app/input.ts";
import { push } from "./app/nav.ts";
import { requestPersistence } from "./app/storage.ts";
import { App } from "./ui/App.tsx";

installInput();
// For the browser UI test (tests/e2e/ui.ts): read the game state like tst_ui does in Qt.
Object.assign(globalThis, { stroopy: { controller, highscores, backup, multiplayer } });
void requestPersistence();

// ?c=<game code> opens "Enter code" with it (docs/design/product.md, "Platforms").
const params = new URLSearchParams(location.search);
const code = params.get("c");
if (code) push({ name: "enterCode", text: code });
// ?j=<invitation> opens the join flow.
const invitation = params.get("j");
if (invitation) push({ name: "multiplayer", text: invitation });
if (params.size) history.replaceState(null, "", location.pathname);
installHistory();

render(<App />, document.getElementById("app")!);

if ("serviceWorker" in navigator && location.protocol === "https:") {
  navigator.serviceWorker.register("./sw.js").catch(() => {});
}
