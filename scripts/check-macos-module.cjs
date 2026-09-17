// Compile the module and the real generated bridge against an existing host's
// CocoaPods headers, without modifying or building that host application.
const fs = require("node:fs");
const path = require("node:path");
const { spawnSync } = require("node:child_process");
const pods = process.argv[2];
if (!pods || !fs.existsSync(path.join(pods, "Headers/Public"))) {
  throw new Error(
    "Usage: node scripts/check-macos-module.cjs /path/to/host/macos/Pods",
  );
}
const publicHeaders = path.resolve(pods, "Headers/Public");
const includes = [
  publicHeaders,
  ...fs
    .readdirSync(publicHeaders)
    .map((name) => path.join(publicHeaders, name)),
  path.resolve(pods, "RCT-Folly"),
  path.resolve(pods, "boost"),
  path.resolve(pods, "fmt/include"),
  path.resolve("node_modules/react-native-macos/ReactCommon"),
  path.resolve("build/codegen/RNSecureStoreSpec"),
];
const result = spawnSync(
  "xcrun",
  [
    "clang++",
    "-fsyntax-only",
    "-std=c++20",
    "-fobjc-arc",
    "-fblocks",
    "-DRCT_NEW_ARCH_ENABLED=1",
    "-DFOLLY_NO_CONFIG=1",
    "-DFOLLY_MOBILE=1",
    "-DFOLLY_USE_LIBCPP=1",
    ...includes.flatMap((include) => ["-I", include]),
    "macos/RNSecureStore.mm",
    "build/codegen/RNSecureStoreSpec/RNSecureStoreSpec/RNSecureStoreSpec-generated.mm",
  ],
  { stdio: "inherit" },
);
process.exit(result.status ?? 1);
