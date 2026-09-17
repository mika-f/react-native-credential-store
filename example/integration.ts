import { SecureStore, SecureStoreError } from "../src";

function check(condition: boolean, label: string) {
  // Error messages deliberately omit expected/actual secret values.
  if (!condition) throw new Error(`SecureStore smoke test failed: ${label}`);
}

/** Run inside a macOS/Windows React Native host after native installation.
 * Resolves with a test count; writes only dummy secrets and cleans them up.
 * To test explicit applicationId, configure it before invoking this function.
 */
export async function runIntegrationTests(): Promise<number> {
  const prefix = `smoke-${Date.now()}-${Math.random().toString(36).slice(2)}`;
  const options = { service: `${prefix}/認証` };
  const entries: Array<[string, { service: string } | undefined]> = [];
  let checks = 0;
  function verify(condition: boolean, label: string) {
    check(condition, label);
    checks++;
  }
  async function set(key: string, value: string, target = options) {
    entries.push([key, target]);
    await SecureStore.setItem(key, value, target);
  }
  try {
    verify(
      (await SecureStore.getItem("missing", options)) === null,
      "missing get",
    );
    verify(
      (await SecureStore.hasItem("missing", options)) === false,
      "missing has",
    );
    await SecureStore.removeItem("missing", options);
    await set("token", "first");
    verify(
      (await SecureStore.getItem("token", options)) === "first",
      "set/get",
    );
    await set("token", "second");
    verify(
      (await SecureStore.getItem("token", options)) === "second",
      "overwrite",
    );
    for (const value of [
      "",
      "認証🔑",
      "a\0b",
      "あ".repeat(853),
      "あ".repeat(853) + "x",
    ]) {
      await set("value", value);
      verify(
        (await SecureStore.getItem("value", options)) === value,
        "value round trip",
      );
      verify(await SecureStore.hasItem("value", options), "value existence");
    }
    try {
      await SecureStore.setItem("value", "あ".repeat(853) + "xx", options);
      throw new Error("Oversized value was accepted");
    } catch (error) {
      verify(
        error instanceof SecureStoreError && error.code === "E_VALUE_TOO_LARGE",
        "byte limit",
      );
    }
    const variants: Array<[string, { service: string }]> = [
      ["token", options],
      ["Token", options],
      ["token", { service: options.service.toUpperCase() }],
      ["é", options],
      ["e\u0301", options],
    ];
    for (let i = 0; i < variants.length; i++)
      await set(variants[i][0], String(i), variants[i][1]);
    for (let i = 0; i < variants.length; i++)
      verify(
        (await SecureStore.getItem(variants[i][0], variants[i][1])) ===
          String(i),
        "namespace isolation",
      );
    const defaultKey = `${prefix}-default`;
    entries.push([defaultKey, undefined]);
    await SecureStore.setItem(defaultKey, "default");
    verify(
      (await SecureStore.getItem(defaultKey, { service: "default" })) ===
        "default",
      "default service",
    );
    await SecureStore.removeItem("token", options);
    await SecureStore.removeItem("token", options);
    verify(
      (await SecureStore.getItem("token", options)) === null &&
        !(await SecureStore.hasItem("token", options)),
      "removal",
    );
    await Promise.all(
      Array.from({ length: 16 }, (_, i) => set("concurrent", String(i))),
    );
    verify(
      await SecureStore.hasItem("concurrent", options),
      "concurrent writes",
    );
    return checks;
  } finally {
    for (const [key, target] of entries)
      await SecureStore.removeItem(key, target);
  }
}
