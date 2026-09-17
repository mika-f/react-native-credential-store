module.exports = {
  dependency: {
    platforms: {
      ios: null,
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
