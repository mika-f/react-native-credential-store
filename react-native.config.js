module.exports = {
  dependency: {
    platforms: {
      // RN macOS 0.81 invokes Apple Codegen with target "ios". Do not disable
      // that key: it would exclude this library from autolinking and the
      // TurboModule provider.
      android: null,
      macos: { podspecPath: "RNSecureStore.podspec" },
      windows: {
        sourceDir: "windows",
        projects: [
          {
            projectFile: "RNSecureStore/RNSecureStore.vcxproj",
            directDependency: true,
          },
        ],
      },
    },
  },
};
