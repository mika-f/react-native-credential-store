const { test } = require("node:test");
const assert = require("node:assert/strict");
const {
  encodeNamespaceComponent: encode,
  buildServiceNamespace,
  buildLogicalUrl,
  utf8ByteLength,
} = require("../lib/namespace");

for (const [input, expected] of [
  ["AZaz09-._~", "AZaz09-._~"],
  ["/", "%2F"],
  [":", "%3A"],
  ["%", "%25"],
  [" ", "%20"],
  ["!'()*", "%21%27%28%29%2A"],
  ["認証", "%E8%AA%8D%E8%A8%BC"],
  ["🔑", "%F0%9F%94%91"],
  ["\0", "%00"],
])
  test(`RFC3986: ${JSON.stringify(input)}`, () =>
    assert.equal(encode(input), expected));

test("canonical URLs keep segment boundaries", () => {
  assert.equal(
    buildLogicalUrl("com.example.app", "oauth/google", "access token"),
    "rnssecurestore://v0/com.example.app/oauth%2Fgoogle/access%20token",
  );
  assert.equal(
    buildServiceNamespace("app", "認証"),
    "rnssecurestore://v0/app/%E8%AA%8D%E8%A8%BC",
  );
  assert.notEqual(
    buildServiceNamespace("a/b", "c"),
    buildServiceNamespace("a", "b/c"),
  );
});
test("preserves exact Unicode and case identity", () => {
  assert.notEqual(encode("token"), encode("Token"));
  assert.notEqual(encode("é"), encode("e\u0301"));
});
test("UTF-8 length agrees with actual encoded byte sequences", () => {
  for (const s of [
    "",
    "abc\0def",
    "é",
    "あ",
    "🔑",
    "あ".repeat(853) + "x",
    "é".repeat(1280),
  ]) {
    assert.equal(utf8ByteLength(s), Buffer.byteLength(s));
  }
});
test("rejects lone surrogates, accepts paired ones", () => {
  for (const s of ["\ud800", "\udfff", "\ud800x", "\ud800\ud800"]) {
    assert.throws(() => utf8ByteLength(s), { code: "E_ENCODING" });
    assert.throws(() => encode(s), { code: "E_ENCODING" });
  }
  assert.equal(utf8ByteLength("😀"), 4);
});
