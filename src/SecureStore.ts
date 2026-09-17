import native from "./NativeSecureStore";
import { createSecureStore } from "./createSecureStore";
import type { SecureStore as SecureStoreAPI } from "./types";

export type SecureStore = SecureStoreAPI;
export const SecureStore = createSecureStore(native);
