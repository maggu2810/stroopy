// SPDX-License-Identifier: GPL-3.0-or-later
// The current multiplayer room and its link to the local game (docs/design/product.md, "Multiplayer").
import { effect, signal, untracked } from "@preact/signals";
import { Mode } from "../core/rules-v1.ts";
import { Room } from "../net/room.ts";
import { randomRoomId } from "../net/protocol-v1.ts";
import { controller } from "./controller.ts";
import { stack } from "./nav.ts";
import { effectiveRelays, playerName } from "./settings.ts";

export const room = signal<Room | null>(null);
let gameCode = "";

function open(id: string, asHost: boolean) {
  room.value?.leave();
  room.value = new Room(id, asHost, playerName.value, effectiveRelays.value, {
    play(code, mode, seed) {
      gameCode = code;
      controller.start(mode as Mode, seed);
      stack.value = [{ name: "home" }, { name: "lobby" }, { name: "game" }];
    },
    busy: () => controller.phase.value === "countdown" || controller.phase.value === "running",
  });
  stack.value = [{ name: "home" }, { name: "lobby" }];
}

export const createRoom = () => open(randomRoomId(), true);
export const joinRoom = (id: string) => open(id, false);

export function leaveRoom() {
  room.value?.leave();
  room.value = null;
  stack.value = [{ name: "home" }];
}

// Report our progress whenever it changes (throttled by the room).
effect(() => {
  const r = room.peek();
  const phase = controller.phase.value;
  const score = controller.score.value;
  void controller.answers.value;
  if (!r || !gameCode || phase === "idle" || r.me.peek().game?.code !== gameCode) return;
  untracked(() => r.report({ code: gameCode, n: controller.n, score, over: phase === "over" }));
});

effect(() => {
  const r = room.value;
  const name = playerName.value;
  untracked(() => r?.setName(name));
});

/** Quitting a multiplayer game counts as being out at the current item. */
export function quitRoomGame() {
  const r = room.value;
  if (r && gameCode && controller.phase.value !== "over") {
    const wasRunning = controller.phase.value === "running";
    controller.abort();
    r.report({ code: gameCode, n: wasRunning ? controller.n : 1, score: controller.score.value, over: true });
  }
  stack.value = [{ name: "home" }, { name: "lobby" }];
}
