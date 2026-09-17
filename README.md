# React Native Credential Store

Secure storage for React Native for macOS / Windows. It stores small strings such as access tokens, passwords, and API keys in the OS-native credential store.

- macOS: Keychain Services (Generic Password)
- Windows: Credential Manager (Generic Credential)
- TypeScript API with a Turbo Native Module
- Namespaces per application / service
- Stores strings up to 2560 UTF-8 bytes

This package is currently in **alpha**. Building on Windows, running against a real Credential Manager, and running within React Native hosts on both platforms have not yet been verified.

## Requirements

| Item | Requirement |
| --- | --- |
| React Native | 0.81 or later, New Architecture |
| macOS | React Native for macOS, macOS 14 or later |
| Windows | React Native for Windows, C++ desktop host, x64 / ARM64 |

iOS, Android, Web, Linux, and Expo Go are not supported. On Windows, a process that can access the logon user's credential store is required.

## Installation

```sh
pnpm add @natsuneko-laboratory/react-native-credential-store
```

### macOS

Update the CocoaPods dependencies in your app's `macos` directory and rebuild the app.

```sh
cd macos
bundle exec pod install
```

If you are not using Bundler, run `pod install` instead. The native module is autolinked, and React Native Codegen generates the bridge.

### Windows

After adding the package, rebuild the app.

```sh
pnpm exec react-native run-windows
```

The package ships with a C++/WinRT package provider and a pre-generated TurboModule spec, and supports RN Windows autolinking. Prepare Visual Studio and the Windows SDK according to the requirements of the RN Windows version you use.

## Quick Start

```ts
import { SecureStore } from "@natsuneko-laboratory/react-native-credential-store";

const options = { service: "authentication" };

await SecureStore.setItem("access-token", "your-access-token", options);

const token = await SecureStore.getItem("access-token", options);
// null if not stored

const exists = await SecureStore.hasItem("access-token", options);

await SecureStore.removeItem("access-token", options);
```

No initial setup is usually required. If `service` is omitted, `"default"` is used.

## API

```ts
interface SecureStoreOptions {
  service?: string;
}

interface SecureStoreConfiguration {
  applicationId?: string;
}

interface SecureStore {
  configure(configuration: SecureStoreConfiguration): void;
  setItem(key: string, value: string, options?: SecureStoreOptions): Promise<void>;
  getItem(key: string, options?: SecureStoreOptions): Promise<string | null>;
  removeItem(key: string, options?: SecureStoreOptions): Promise<void>;
  hasItem(key: string, options?: SecureStoreOptions): Promise<boolean>;
}
```

| Method | Behavior |
| --- | --- |
| `configure` | Sets the application ID before the first storage operation. |
| `setItem` | Stores the value. Overwrites the value if the same namespace and key exist. |
| `getItem` | Returns the stored string, or `null` if it does not exist. |
| `removeItem` | Removes the credential. Removing a non-existent key also succeeds. |
| `hasItem` | Returns `true` if it exists, `false` otherwise. |

`key`, `service`, and `applicationId` must be non-empty strings. An empty string can be stored as a value and is distinguished from an unset `null`.

The value limit is **2560 UTF-8 bytes**. It is not measured in JavaScript characters. For example, `"あ"` is 3 bytes and `"🔑"` is 4 bytes. Values other than strings, and strings that cannot be converted to UTF-8 correctly (such as lone surrogates), are rejected.

## Namespaces and application ID

Credentials are identified by the combination of `(applicationId, service, key)`. Each is case-sensitive, with no Unicode normalization or whitespace trimming. The same key becomes a different credential if the service differs.

```ts
await SecureStore.setItem("token", "work-token", { service: "work" });
await SecureStore.setItem("token", "personal-token", { service: "personal" });
```

The application ID is resolved automatically, preferring the bundle identifier on macOS and the Package Family Name on Windows. If unavailable, it falls back to the application metadata, and finally to `local.<executable-name>`. The resolved ID is cached.

To avoid being affected by changes to the app name or metadata, specify a stable ID explicitly.

```ts
import { SecureStore } from "@natsuneko-laboratory/react-native-credential-store";

// Run at app startup, before the first storage operation.
SecureStore.configure({ applicationId: "com.example.desktop" });
```

The configuration is locked once any of `setItem`, `getItem`, `removeItem`, or `hasItem` is called. Even if that operation fails, subsequent calls to `configure` throw `E_CONFIGURATION_LOCKED`.

Keep the application ID consistent across app updates. Changing it points to a different namespace, making existing credentials unreachable. The namespace is for organization and identification only; the OS manages access permissions.

## Error handling

Storage operation errors are reported via Promise rejection, and `configure` errors are thrown synchronously.

```ts
import {
  SecureStore,
  SecureStoreError,
} from "@natsuneko-laboratory/react-native-credential-store";

async function readToken() {
  try {
    return await SecureStore.getItem("access-token", {
      service: "authentication",
    });
  } catch (error) {
    if (error instanceof SecureStoreError && error.code === "E_ACCESS_DENIED") {
      // Guide the user to check access permissions as needed.
    }
    throw error;
  }
}
```

| `code` | Meaning |
| --- | --- |
| `E_CONFIGURATION_LOCKED` | Attempted to change the configuration after a storage operation has started |
| `E_INVALID_APPLICATION_ID` | The application ID is empty or not a string |
| `E_INVALID_KEY` | The key is empty or not a string |
| `E_INVALID_SERVICE` | The service or options is invalid |
| `E_VALUE_TOO_LARGE` | The value exceeds 2560 UTF-8 bytes |
| `E_ENCODING` | The value is not a string, or failed to convert to UTF-8 |
| `E_ACCESS_DENIED` | The OS denied access to the storage |
| `E_PLATFORM` | The OS storage operation failed |
| `E_UNKNOWN` | An unexpected error occurred |

If an OS error code is available, `nativeCode` holds the macOS OSStatus or Windows Win32 error. This is for diagnostics; use `code` for application-level branching.

## Storage format and security

Encryption and persistence are delegated to the OS credential store. The library does not store values in plaintext files or AsyncStorage, and does not manage its own encryption keys. Credential values are never written to logs or error messages.

| Platform | Storage and identification |
| --- | --- |
| macOS | Generic Password. The service is `rnssecurestore://v0/<encoded-app>/<encoded-service>`, and the account is the original key. |
| Windows | Generic Credential. The TargetName is `rnssecurestore://v0/<appHash>/<serviceHash>/<keyHash>`. |

On macOS, each namespace element is percent-encoded as UTF-8 according to RFC 3986. On Windows, each hash is the lowercase hex representation of SHA-256 over the UTF-8 bytes of the original string. This preserves logical case differences even on Windows, where TargetName is case-insensitive. `v0` is the storage format version, independent of the package version.

macOS disables Keychain synchronization, and Windows stores with `CRED_PERSIST_LOCAL_MACHINE` for the current user and machine. No Keychain Access Group is added. The host app's code signing and sandbox settings, along with OS access control, apply.

Storage operations are processed in the background. Ordering of multiple concurrent writes, and transactions bundling multiple credentials, are not guaranteed. Also, erasure of all memory copies within JS engines or runtimes is not guaranteed.

Binary data, large-capacity storage, key enumeration, deleting all credentials, biometric authentication, and cross-device synchronization are not provided.

## Development

```sh
pnpm install
pnpm typecheck
pnpm test
pnpm format:check
```

If you change the native API spec, run Codegen. The pre-generated Windows headers are included in the repository and the distribution package.

```sh
pnpm codegen:macos
pnpm codegen:windows
```

macOS native tests:

```sh
pnpm test:macos
pnpm test:macos --integration
```

Windows native tests (Visual Studio C++ development environment):

```sh
cmake -S tests/native -B build/native-windows -A x64
cmake --build build/native-windows --config Debug
ctest --test-dir build/native-windows -C Debug --output-on-failure
```

Integration tests store dummy values in a test-only namespace and remove them on exit. See [example/tokens.ts](example/tokens.ts) for usage, and [example/integration.ts](example/integration.ts) for tests that run within a React Native host.

To create a distribution package from source, run `pnpm pack`. You can install the generated `.tgz` in your app with `pnpm add /path/to/package.tgz`.

## License

[MIT](LICENSE)
