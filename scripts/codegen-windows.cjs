const { runCodeGen } = require("@react-native-windows/codegen");
const config = require("../package.json").codegenConfig;
process.chdir(require("node:path").join(__dirname, ".."));
runCodeGen({
  files: ["src/NativeSecureStore.ts"],
  libraryName: config.name,
  modulesWindows: true,
  namespace: config.windows.namespace,
  outputDirectory: config.windows.outputDirectory,
  separateDataTypes: true,
  cppStringType: "std::string",
});
