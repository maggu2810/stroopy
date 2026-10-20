// SPDX-License-Identifier: GPL-3.0-or-later
// Loads conformance vectors from docs/spec/test-vectors/.
import { join } from "node:path";

const root = join(import.meta.dir, "../../../docs/spec/test-vectors");

// eslint-disable-next-line @typescript-eslint/no-explicit-any
export async function vectors(area: string, file: string): Promise<any> {
  return Bun.file(join(root, area, file)).json();
}

export const u64 = (hex: string) => BigInt(hex);
export const hex = (bytes: Uint8Array) => Buffer.from(bytes).toString("hex");
