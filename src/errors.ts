export type SecureStoreErrorCode =
  | "E_CONFIGURATION_LOCKED"
  | "E_INVALID_APPLICATION_ID"
  | "E_INVALID_KEY"
  | "E_INVALID_SERVICE"
  | "E_VALUE_TOO_LARGE"
  | "E_ENCODING"
  | "E_ACCESS_DENIED"
  | "E_PLATFORM"
  | "E_UNKNOWN";

const messages: Record<SecureStoreErrorCode, string> = {
  E_CONFIGURATION_LOCKED: "SecureStore configuration is locked.",
  E_INVALID_APPLICATION_ID: "applicationId must be a non-empty string.",
  E_INVALID_KEY: "key must be a non-empty string.",
  E_INVALID_SERVICE: "service must be a non-empty string.",
  E_VALUE_TOO_LARGE: "value exceeds 2560 UTF-8 bytes.",
  E_ENCODING: "A string could not be encoded or decoded as valid UTF-8.",
  E_ACCESS_DENIED: "Access to the credential store was denied.",
  E_PLATFORM: "The native credential store operation failed.",
  E_UNKNOWN: "An unknown credential store error occurred.",
};

export class SecureStoreError extends Error {
  readonly code: SecureStoreErrorCode;
  readonly nativeCode?: number;

  constructor(
    code: SecureStoreErrorCode,
    message = messages[code],
    nativeCode?: number,
  ) {
    super(message);
    this.name = "SecureStoreError";
    this.code = code;
    this.nativeCode = nativeCode;
  }
}

export function normalizeError(error: unknown): SecureStoreError {
  if (error instanceof SecureStoreError) return error;
  const record =
    error !== null && typeof error === "object"
      ? (error as Record<string, unknown>)
      : {};
  const code =
    typeof record.code === "string" &&
    Object.hasOwnProperty.call(messages, record.code)
      ? (record.code as SecureStoreErrorCode)
      : "E_UNKNOWN";
  // Apple bridges NSError.userInfo, RNW bridges ReactError.UserInfo.
  const info =
    record.userInfo !== null && typeof record.userInfo === "object"
      ? (record.userInfo as Record<string, unknown>)
      : {};
  const raw = record.nativeCode ?? info.nativeCode;
  const nativeCode =
    typeof raw === "number" && Number.isFinite(raw) ? raw : undefined;
  // Never forward untrusted native messages or causes: they could contain a secret.
  return new SecureStoreError(code, messages[code], nativeCode);
}
