import { SecureStore } from "../src";

// Usually automatic bundle/package identity is sufficient. Configure before
// any operation only when an explicit, stable namespace is needed.
export function configureCredentials() {
  SecureStore.configure({ applicationId: "com.example.desktop" });
}

export async function saveTokens(accessToken: string, refreshToken: string) {
  const options = { service: "authentication" };
  await SecureStore.setItem("access-token", accessToken, options);
  await SecureStore.setItem("refresh-token", refreshToken, options);
}

export async function restoreTokens() {
  const options = { service: "authentication" };
  const accessToken = await SecureStore.getItem("access-token", options);
  const refreshToken = await SecureStore.getItem("refresh-token", options);
  return accessToken === null || refreshToken === null
    ? null
    : { accessToken, refreshToken };
}

export async function removeTokens() {
  const options = { service: "authentication" };
  await SecureStore.removeItem("access-token", options);
  await SecureStore.removeItem("refresh-token", options);
}
