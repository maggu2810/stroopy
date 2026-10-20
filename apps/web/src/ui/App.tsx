// SPDX-License-Identifier: GPL-3.0-or-later
import { Mode, RULES_VERSION } from "../core/rules-v1.ts";
import { controller } from "../app/controller.ts";
import { submit } from "../app/highscores.ts";
import { reset, stack } from "../app/nav.ts";
import { BackupPage } from "./BackupPage.tsx";
import { Toast } from "./common.tsx";
import { EnterCodePage } from "./EnterCodePage.tsx";
import { GamePage } from "./GamePage.tsx";
import { HelpPage } from "./HelpPage.tsx";
import { HighscoresPage } from "./HighscoresPage.tsx";
import { HomePage } from "./HomePage.tsx";
import { ResultPage } from "./ResultPage.tsx";
import { SettingsPage } from "./SettingsPage.tsx";

export function playGame(mode: Mode, seed?: bigint, result?: { score: number; reached: number }, isChallenge = false) {
  controller.start(mode, seed, result, isChallenge);
  reset({ name: "game" });
}

controller.onOver(() => {
  const rank = submit({
    mode: controller.mode.value,
    rules: RULES_VERSION,
    seed: controller.seedHex,
    score: controller.score.value,
    reached: controller.reached,
    challenge: controller.isChallenge.value,
  });
  reset({ name: "result", rank });
});

function quitGame() {
  controller.abort();
  reset();
}

function CurrentPage() {
  const page = stack.value.at(-1)!;
  switch (page.name) {
    case "home":
      return <HomePage onPlay={(mode) => playGame(mode)} />;
    case "game":
      return <GamePage onQuit={quitGame} />;
    case "result":
      return (
        <ResultPage
          rank={page.rank}
          onReplay={() => {
            controller.restart();
            reset({ name: "game" });
          }}
          onNewGame={() => playGame(controller.mode.value)}
          onHome={() => reset()}
        />
      );
    case "highscores":
      return <HighscoresPage onPlay={(mode, seed) => playGame(mode as Mode, seed)} />;
    case "enterCode":
      return <EnterCodePage text={page.text} onPlay={(mode, seed, result) => playGame(mode as Mode, seed, result, true)} />;
    case "settings":
      return <SettingsPage />;
    case "backup":
      return <BackupPage />;
    case "help":
      return <HelpPage />;
  }
}

export function App() {
  return (
    <>
      <CurrentPage key={stack.value.length + ":" + stack.value.at(-1)!.name} />
      <Toast />
    </>
  );
}
