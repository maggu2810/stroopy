// SPDX-License-Identifier: GPL-3.0-or-later
// Export and import of the backup code (docs/design/product.md, "Backup").
import { useState } from "preact/hooks";
import { decodeText } from "../core/backup-v1.ts";
import { exportCode, importBackup } from "../app/backup.ts";
import { t } from "../app/i18n.ts";
import { pop } from "../app/nav.ts";
import { modeName } from "../app/theme.ts";
import { copyText, pasteText, shareText, SubPage, toast } from "./common.tsx";

export function BackupPage() {
  const [input, setInput] = useState("");
  const [restoreSettings, setRestoreSettings] = useState(true);
  const code = exportCode();
  const d = decodeText(input);

  let preview: string[] = [];
  if (!d.ok) preview = [d.kind === "unsupported" ? t("This backup needs a newer version of Stroopy.") : t("No valid backup found.")];
  else {
    const counts = new Map<number, number>();
    for (const e of d.value.entries) counts.set(e.mode, (counts.get(e.mode) ?? 0) + 1);
    preview = [t("Highscores: %1", d.value.entries.length), ...[...counts].sort(([a], [b]) => a - b).map(([m, n]) => `  ${modeName(m)}: ${n}`)];
    preview.push(d.value.settings ? t("Settings included") : t("No settings included"));
  }

  return (
    <SubPage title={t("Backup")} onBack={pop} class="backup">
      <div class="two-col top">
        <section class="column">
          <h3>{t("Export")}</h3>
          <p class="muted">
            {t("All highscores and settings in one code. Keep it somewhere safe, e.g. in a note or a message to yourself, and import it on another device or in another Stroopy app.")}
          </p>
          <textarea class="mono" readOnly rows={5} value={code} tabIndex={-1} />
          <div class="row">
            <button data-autofocus onClick={async () => toast((await copyText(code)) ? t("Backup copied.") : t("Copying failed."))}>
              {t("Copy")}
            </button>
            {"share" in navigator && <button onClick={() => shareText(code)}>{t("Share")}</button>}
          </div>
        </section>
        <section class="column">
          <h3>{t("Import")}</h3>
          <textarea rows={5} placeholder="SYB1-…" spellcheck={false} autocapitalize="off" value={input} onInput={(e) => setInput(e.currentTarget.value)} />
          <button class="inline" onClick={async () => setInput((await pasteText()) ?? input)}>
            {t("Paste")}
          </button>
          {input && <p class={`status ${d.ok ? "" : "bad"}`}>{preview.join("\n")}</p>}
          <label class="switch">
            <input type="checkbox" checked={restoreSettings} disabled={!(d.ok && d.value.settings)} onChange={(e) => setRestoreSettings(e.currentTarget.checked)} />
            <span>{t("Also restore settings")}</span>
          </label>
          <button
            class="primary"
            disabled={!d.ok}
            onClick={() => {
              if (!d.ok) return;
              toast(t("Import done. New highscores: %1", importBackup(d.value, restoreSettings)));
              setInput("");
            }}
          >
            {t("Import")}
          </button>
        </section>
      </div>
    </SubPage>
  );
}
