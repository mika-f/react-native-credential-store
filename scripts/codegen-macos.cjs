const path = require("node:path");
const { createRequire } = require("node:module");
const rnRequire = createRequire(
  require.resolve("react-native-macos/package.json"),
);
const { TypeScriptParser } = rnRequire(
  "@react-native/codegen/lib/parsers/typescript/parser",
);
const { generateSpecFromInMemorySchema } = rnRequire(
  "./scripts/codegen/generate-specs-cli-executor",
);
const root = path.join(__dirname, "..");
const schema = new TypeScriptParser().parseFile(
  path.join(root, "src/NativeSecureStore.ts"),
);
generateSpecFromInMemorySchema(
  "ios",
  schema,
  path.join(root, "build/codegen/RNSecureStoreSpec"),
  "RNSecureStoreSpec",
  undefined,
  "modules",
  true,
);
