const { test } = require("node:test");
const assert = require("node:assert/strict");
const { createSecureStore } = require("../lib/createSecureStore");
const { SecureStoreError, normalizeError } = require("../lib/errors");

function fixture(overrides = {}) {
  const data = new Map();
  const calls = [];
  const tuple = (key, service, app) => JSON.stringify([app, service, key]);
  const native = {
    async getDefaultApplicationId() {
      calls.push("identity");
      return "com.test.app";
    },
    async setItem(key, value, service, app) {
      calls.push(["set", app, service, key]);
      data.set(tuple(key, service, app), value);
    },
    async getItem(key, service, app) {
      return data.get(tuple(key, service, app)) ?? null;
    },
    async removeItem(key, service, app) {
      data.delete(tuple(key, service, app));
    },
    async hasItem(key, service, app) {
      return data.has(tuple(key, service, app));
    },
    ...overrides,
  };
  return { store: createSecureStore(native), native, data, calls };
}

test("CRUD, upsert, existence and idempotent removal", async () => {
  const { store } = fixture();
  assert.equal(await store.getItem("key"), null);
  assert.equal(await store.hasItem("key"), false);
  assert.equal(await store.setItem("key", "one"), undefined);
  assert.equal(await store.hasItem("key"), true);
  assert.equal(await store.getItem("key"), "one");
  await store.setItem("key", "two");
  assert.equal(await store.getItem("key"), "two");
  await store.removeItem("key");
  await store.removeItem("key");
  assert.equal(await store.getItem("key"), null);
});
test("empty, Unicode and embedded NUL values round-trip", async () => {
  const { store } = fixture();
  for (const value of ["", "認証🔑", "a\0b"]) {
    await store.setItem("key", value);
    assert.equal(await store.hasItem("key"), true);
    assert.equal(await store.getItem("key"), value);
  }
});
test("default service and exact identifiers", async () => {
  const { store } = fixture();
  await store.setItem("token", "default");
  assert.equal(await store.getItem("token", { service: "default" }), "default");
  for (const [key, service] of [
    ["Token", "default"],
    ["token", "Default"],
    ["é", "認証"],
    ["e\u0301", "認証"],
    ["token", "a/b"],
  ]) {
    await store.setItem(key, `${key}:${service}`, { service });
    assert.equal(await store.getItem(key, { service }), `${key}:${service}`);
  }
  assert.equal(await store.getItem("token"), "default");
});
test("explicit application override is copied and automatic lookup is bypassed", async () => {
  const { store, native, calls } = fixture();
  const config = { applicationId: "App" };
  store.configure(config);
  config.applicationId = "changed";
  await store.setItem("token", "one");
  const other = createSecureStore(native);
  other.configure({ applicationId: "app" });
  assert.equal(await other.getItem("token"), null);
  assert.equal(calls.includes("identity"), false);
  assert.deepEqual(calls[0], ["set", "App", "default", "token"]);
});
test("configure can reset an override before storage begins", async () => {
  const { store, calls } = fixture();
  store.configure({ applicationId: "override" });
  store.configure({});
  await store.getItem("key");
  assert.deepEqual(calls, ["identity"]);
});
test("concurrent requests share one in-flight identity resolution", async () => {
  let resolve;
  let lookups = 0;
  const { store } = fixture({
    getDefaultApplicationId() {
      lookups++;
      return new Promise((r) => {
        resolve = r;
      });
    },
  });
  const pending = [
    store.getItem("a"),
    store.hasItem("b"),
    store.setItem("c", "value"),
  ];
  assert.throws(() => store.configure({}), { code: "E_CONFIGURATION_LOCKED" });
  await Promise.resolve();
  assert.equal(lookups, 1);
  resolve("app");
  await Promise.all(pending);
  await store.getItem("a");
  assert.equal(lookups, 1);
});
for (const method of ["setItem", "getItem", "hasItem", "removeItem"]) {
  test(`${method} locks immediately and rejects invalid calls asynchronously`, async () => {
    const { store } = fixture();
    const promise =
      method === "setItem" ? store[method]("", "value") : store[method]("");
    assert.ok(promise instanceof Promise);
    assert.throws(() => store.configure({}), {
      code: "E_CONFIGURATION_LOCKED",
    });
    await assert.rejects(promise, { code: "E_INVALID_KEY" });
  });
}
test("all key, service and applicationId invalid runtime values are rejected", async () => {
  for (const value of ["", null, 42, {}, []]) {
    const { store } = fixture();
    await assert.rejects(store.getItem(value), { code: "E_INVALID_KEY" });
    await assert.rejects(store.getItem("key", { service: value }), {
      code: "E_INVALID_SERVICE",
    });
    assert.throws(() => fixture().store.configure({ applicationId: value }), {
      code: "E_INVALID_APPLICATION_ID",
    });
  }
  for (const options of [null, 1, "service", []])
    await assert.rejects(fixture().store.getItem("key", options), {
      code: "E_INVALID_SERVICE",
    });
  for (const config of [null, undefined, 1, []])
    assert.throws(() => fixture().store.configure(config), {
      code: "E_INVALID_APPLICATION_ID",
    });
});
test("invalid values and ill-formed UTF-16 never reach native storage", async () => {
  const { store, data } = fixture();
  for (const value of [null, undefined, 12, {}, "\ud800"])
    await assert.rejects(store.setItem("key", value), { code: "E_ENCODING" });
  await assert.rejects(store.getItem("\udfff"), { code: "E_ENCODING" });
  await assert.rejects(store.getItem("key", { service: "\udfff" }), {
    code: "E_ENCODING",
  });
  assert.throws(() => fixture().store.configure({ applicationId: "\ud800" }), {
    code: "E_ENCODING",
  });
  assert.equal(data.size, 0);
});
for (const [bytes, value] of [
  [2559, "あ".repeat(853)],
  [2560, "あ".repeat(853) + "x"],
  [2561, "あ".repeat(853) + "xx"],
]) {
  test(`${bytes} UTF-8 byte boundary`, async () => {
    const { store, data } = fixture();
    assert.equal(Buffer.byteLength(value), bytes);
    if (bytes > 2560) {
      await assert.rejects(store.setItem("key", value), {
        code: "E_VALUE_TOO_LARGE",
      });
      assert.equal(data.size, 0);
    } else {
      await store.setItem("key", value);
      assert.equal(await store.getItem("key"), value);
    }
  });
}
test("identity errors are normalized and failed resolution is cached", async () => {
  let calls = 0;
  const { store } = fixture({
    getDefaultApplicationId() {
      calls++;
      throw {
        code: "E_PLATFORM",
        userInfo: { nativeCode: 1312 },
        message: "secret",
      };
    },
  });
  for (let i = 0; i < 2; i++)
    await assert.rejects(store.getItem("key"), {
      code: "E_PLATFORM",
      nativeCode: 1312,
    });
  assert.equal(calls, 1);
  for (const id of ["", null, 42])
    await assert.rejects(
      fixture({
        async getDefaultApplicationId() {
          return id;
        },
      }).store.getItem("key"),
      { code: "E_INVALID_APPLICATION_ID" },
    );
});
test("error normalization preserves only recognized codes and numeric diagnostics", async () => {
  for (const native of [
    { code: "E_ACCESS_DENIED", nativeCode: -25293, message: "secret" },
    {
      code: "E_ACCESS_DENIED",
      userInfo: { nativeCode: -25293 },
      message: "secret",
    },
  ]) {
    const { store } = fixture({
      getItem() {
        throw native;
      },
    });
    await assert.rejects(store.getItem("key"), (error) => {
      assert.ok(error instanceof SecureStoreError);
      assert.equal(error.code, "E_ACCESS_DENIED");
      assert.equal(error.nativeCode, -25293);
      assert.equal(error.message.includes("secret"), false);
      assert.equal(error.cause, undefined);
      return true;
    });
  }
  for (const value of [
    null,
    "secret",
    { code: "constructor" },
    { code: "NOPE" },
  ])
    assert.equal(normalizeError(value).code, "E_UNKNOWN");
  assert.equal(
    normalizeError({ code: "E_PLATFORM", nativeCode: "secret" }).nativeCode,
    undefined,
  );
});
