import { SecureStoreError } from "./errors";

export const MAX_VALUE_BYTES = 2560;

// Hermes does not require a TextEncoder polyfill. Reject lone surrogates so
// different logical strings can never collapse to the replacement character.
export function utf8ByteLength(value: string): number {
  let bytes = 0;
  for (let i = 0; i < value.length; i++) {
    const unit = value.charCodeAt(i);
    if (unit < 0x80) bytes++;
    else if (unit < 0x800) bytes += 2;
    else if (unit >= 0xd800 && unit <= 0xdbff) {
      const next = value.charCodeAt(++i);
      if (!(next >= 0xdc00 && next <= 0xdfff))
        throw new SecureStoreError("E_ENCODING");
      bytes += 4;
    } else if (unit >= 0xdc00 && unit <= 0xdfff) {
      throw new SecureStoreError("E_ENCODING");
    } else bytes += 3;
  }
  return bytes;
}

export function encodeNamespaceComponent(value: string): string {
  utf8ByteLength(value);
  return encodeURIComponent(value).replace(
    /[!'()*]/g,
    (c) => `%${c.charCodeAt(0).toString(16).toUpperCase()}`,
  );
}

export function buildServiceNamespace(
  applicationId: string,
  service: string,
): string {
  return `rnssecurestore://v0/${encodeNamespaceComponent(applicationId)}/${encodeNamespaceComponent(service)}`;
}

export function buildLogicalUrl(
  applicationId: string,
  service: string,
  key: string,
): string {
  return `${buildServiceNamespace(applicationId, service)}/${encodeNamespaceComponent(key)}`;
}
