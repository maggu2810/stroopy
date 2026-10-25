// SPDX-License-Identifier: GPL-3.0-or-later
// Room lobby with the ranking of the last game (docs/design/product.md, "Multiplayer").
import { MODES, RULES_VERSION } from "../core/rules-v1.ts";
import { decodeText } from "../core/game-v1.ts";
import { t } from "../app/i18n.ts";
import { leaveRoom, room } from "../app/multiplayer.ts";
import { modeName } from "../app/theme.ts";
import type { Player } from "../net/room.ts";
import { shareText, SubPage, toast } from "./common.tsx";

const rank = (a: Player, b: Player) => (b.game?.score ?? 0) - (a.game?.score ?? 0) || (b.game?.n ?? 0) - (a.game?.n ?? 0);

export function LobbyPage() {
  const r = room.value;
  if (!r) return null;
  const everyone = r.everyone.value;
  const me = r.me.value;
  const isHost = r.isHost.value;
  const connected = r.connected.value;
  const lastCode = me.game?.code;
  const last = lastCode ? decodeText(lastCode) : null;
  const ranking = lastCode ? [...everyone, ...r.departed.value].filter((p) => p.game?.code === lastCode).sort(rank) : [];
  const hostName = everyone.find((p) => p.key === r.hostKey.value)?.name;
  const canStart = isHost && connected > 0 && r.commonRules.value.includes(RULES_VERSION);

  async function share() {
    const link = `${location.origin}${location.pathname}?j=${r!.invitation}`;
    if ((await shareText(t("Play Stroopy with me: %1", link))) === "copied") toast(t("Invitation copied to the clipboard"));
  }

  return (
    <SubPage title={t("Room")} onBack={leaveRoom} class="lobby">
      <div class="two-col top">
        <section class="column">
          <div class="row">
            <button data-autofocus={!isHost || undefined} onClick={share}>
              {t("Invite")}
            </button>
            <span class={`relays ${connected ? "" : "bad"}`}>{t("Relays: %1/%2", connected, r.relayCount)}</span>
          </div>
          <p class="muted room-relays">{r.relays.map((url) => url.replace(/^wss?:\/\//, "")).join(" · ")}</p>
          {ranking.length > 0 && (
            <>
              <h3>{t("Last game: %1", last?.ok ? modeName(last.value.mode) : "")}</h3>
              <ol class="ranking">
                {ranking.map((p) => (
                  <li class={p.me ? "me" : ""}>
                    <span class="name">{p.me ? t("%1 (you)", p.name) : p.name}</span>
                    <span class="points">{p.game!.score}</span>
                    <span class="muted state">{p.game!.over ? t("out at #%1", p.game!.n) : t("playing #%1", p.game!.n)}</span>
                  </li>
                ))}
              </ol>
            </>
          )}
          <h3>{t("Players")}</h3>
          <ul class="players">
            {everyone.map((p) => (
              <li>
                {p.key === r.hostKey.value && <span title={t("Host")}>★ </span>}
                {p.me ? t("%1 (you)", p.name) : p.name}
              </li>
            ))}
          </ul>
          {everyone.length === 1 && <p class="muted">{t("Nobody else is here yet. Invite others with the invitation.")}</p>}
        </section>
        <section class="column">
          {r.pending.value && (
            <button class="primary tall" data-autofocus onClick={() => r.joinPending()}>
              {t("Join the running game")}
            </button>
          )}
          {isHost ? (
            <>
              <h3>{t("Mode")}</h3>
              <div class="mode-pick" role="radiogroup">
                {MODES.map((m) => (
                  <button role="radio" aria-checked={r.mode.value === m} class={r.mode.value === m ? "active" : ""} onClick={() => r.setMode(m)}>
                    {modeName(m)}
                  </button>
                ))}
              </div>
              <button class="primary tall" data-autofocus disabled={!canStart} onClick={() => r.start(r.mode.value)}>
                {t("Start")}
              </button>
            </>
          ) : (
            <p class="status">{hostName ? t("Waiting for %1 to start: %2", hostName, modeName(r.mode.value)) : t("Waiting for the host…")}</p>
          )}
          <button class="flat bad" onClick={leaveRoom}>
            {t("Leave room")}
          </button>
        </section>
      </div>
    </SubPage>
  );
}
