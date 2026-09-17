import type { Spec } from "./NativeSecureStore";
import {
  normalizeError,
  SecureStoreError,
  type SecureStoreErrorCode,
} from "./errors";
import { MAX_VALUE_BYTES, utf8ByteLength } from "./namespace";
import type { SecureStore, SecureStoreOptions } from "./types";

function identifier(
  value: unknown,
  code: SecureStoreErrorCode,
): asserts value is string {
  if (typeof value !== "string" || value.length === 0)
    throw new SecureStoreError(code);
  utf8ByteLength(value);
}

// Internal factory gives each JS runtime one isolated configuration/cache.
export function createSecureStore(native: Spec): SecureStore {
  let locked = false;
  let explicitId: string | undefined;
  let applicationId: Promise<string> | undefined;

  function resolveId(): Promise<string> {
    applicationId ??= Promise.resolve().then(async () => {
      const value = explicitId ?? (await native.getDefaultApplicationId());
      identifier(value, "E_INVALID_APPLICATION_ID");
      return value;
    });
    return applicationId;
  }

  async function operate<T>(
    key: string,
    options: SecureStoreOptions | undefined,
    operation: (id: string, service: string) => Promise<T>,
    validate?: () => void,
  ): Promise<T> {
    // Runs synchronously before the first await, including invalid attempts.
    locked = true;
    try {
      identifier(key, "E_INVALID_KEY");
      if (
        options !== undefined &&
        (options === null ||
          typeof options !== "object" ||
          Array.isArray(options))
      ) {
        throw new SecureStoreError("E_INVALID_SERVICE");
      }
      const service =
        options?.service === undefined ? "default" : options.service;
      identifier(service, "E_INVALID_SERVICE");
      validate?.();
      return await operation(await resolveId(), service);
    } catch (error) {
      throw normalizeError(error);
    }
  }

  return {
    configure(configuration) {
      if (locked) throw new SecureStoreError("E_CONFIGURATION_LOCKED");
      if (
        configuration === null ||
        typeof configuration !== "object" ||
        Array.isArray(configuration)
      ) {
        throw new SecureStoreError("E_INVALID_APPLICATION_ID");
      }
      if (configuration.applicationId !== undefined)
        identifier(configuration.applicationId, "E_INVALID_APPLICATION_ID");
      explicitId = configuration.applicationId;
    },
    setItem(key, value, options) {
      return operate(
        key,
        options,
        (id, service) => native.setItem(key, value, service, id),
        () => {
          if (typeof value !== "string")
            throw new SecureStoreError("E_ENCODING");
          if (utf8ByteLength(value) > MAX_VALUE_BYTES)
            throw new SecureStoreError("E_VALUE_TOO_LARGE");
        },
      );
    },
    getItem(key, options) {
      return operate(key, options, (id, service) =>
        native.getItem(key, service, id),
      );
    },
    removeItem(key, options) {
      return operate(key, options, (id, service) =>
        native.removeItem(key, service, id),
      );
    },
    hasItem(key, options) {
      return operate(key, options, (id, service) =>
        native.hasItem(key, service, id),
      );
    },
  };
}
