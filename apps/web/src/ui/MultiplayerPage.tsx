// SPDX-License-Identifier: GPL-3.0-or-later
// Create or join a room (docs/design/product.md, "Multiplayer").
import { useState } from "preact/hooks";
import { parseRoom } from "../net/protocol-v1.ts";
import { t } from "../app/i18n.ts";
import { createRoom, joinRoom } from "../app/multiplayer.ts";
import { pop } from "../app/nav.ts";
import { playerName, setPlayerName } from "../app/settings.ts";
import { pasteText, SubPage } from "./common.tsx";

export function MultiplayerPage(props: { text?: string }) {
  const [text, setText] = useState(props.text ?? "");
  const parsed = parseRoom(text);
  const join = () => parsed.ok && joinRoom(parsed.value);

  return (
    <SubPage title={t("Multiplayer")} onBack={pop} class="multiplayer">
      <div class="column">
        <p class="muted">{t("Play the same sequence at the same time, over the internet. Everyone sees how far the others get.")}</p>
        <label class="field">
          <span class="muted">{t("Player name")}</span>
          <input type="text" maxLength={24} value={playerName.value} onChange={(e) => setPlayerName(e.currentTarget.value)} />
          <small class="muted">{t("Shown to other players. Names and scores travel unencrypted through public Nostr relays.")}</small>
        </label>
        <button class="primary tall gap" data-autofocus={!props.text || undefined} onClick={createRoom}>
          {t("Create room")}
        </button>
        <h3>{t("Join room")}</h3>
        <input
          type="text"
          placeholder="SYR1-…"
          spellcheck={false}
          autocapitalize="characters"
          autocomplete="off"
          data-autofocus={props.text ? true : undefined}
          value={text}
          onInput={(e) => setText(e.currentTarget.value)}
          onKeyDown={(e) => {
            if (e.key === "Enter") join();
          }}
        />
        <div class="row">
          <button onClick={async () => setText((await pasteText()) ?? text)}>{t("Paste")}</button>
          <button class="primary" disabled={!parsed.ok} onClick={join}>
            {t("Join")}
          </button>
        </div>
        {text && !parsed.ok && (
          <p class="status bad">{parsed.kind === "unsupported" ? t("This room needs a newer version of Stroopy.") : t("No valid room code found.")}</p>
        )}
      </div>
    </SubPage>
  );
}
