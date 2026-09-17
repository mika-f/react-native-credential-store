#include "Namespace.h"
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <limits>
#include <memory>

namespace rnssecurestore {
void validateUtf8(const std::string &value) {
  if (value.empty())
    return;
  if (value.size() > static_cast<size_t>((std::numeric_limits<int>::max)()) ||
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                          static_cast<int>(value.size()), nullptr, 0) == 0) {
    throw Error("E_ENCODING", ERROR_NO_UNICODE_TRANSLATION);
  }
}

void validateIdentity(const std::string &app, const std::string &service, const std::string &key) {
  if (app.empty())
    throw Error("E_INVALID_APPLICATION_ID");
  if (service.empty())
    throw Error("E_INVALID_SERVICE");
  if (key.empty())
    throw Error("E_INVALID_KEY");
  validateUtf8(app);
  validateUtf8(service);
  validateUtf8(key);
}

std::string hashNamespaceComponent(const std::string &value) {
  validateUtf8(value);
  struct Algorithm {
    BCRYPT_ALG_HANDLE handle{};
    ~Algorithm() {
      if (handle)
        BCryptCloseAlgorithmProvider(handle, 0);
    }
  } algorithm;
  struct Hash {
    BCRYPT_HASH_HANDLE handle{};
    ~Hash() {
      if (handle)
        BCryptDestroyHash(handle);
    }
  } hash;
  // CNG returns NTSTATUS, not GetLastError. Translate it before preserving it.
  const auto check = [](NTSTATUS status) {
    if (status < 0) {
      using Convert = ULONG(WINAPI *)(NTSTATUS);
      auto convert = reinterpret_cast<Convert>(
          GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlNtStatusToDosError"));
      throw Error("E_PLATFORM", convert ? convert(status) : ERROR_GEN_FAILURE);
    }
  };
  check(BCryptOpenAlgorithmProvider(&algorithm.handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
  check(BCryptCreateHash(algorithm.handle, &hash.handle, nullptr, 0, nullptr, 0, 0));
  if (!value.empty())
    check(BCryptHashData(hash.handle, reinterpret_cast<PUCHAR>(const_cast<char *>(value.data())),
                         static_cast<ULONG>(value.size()), 0));
  std::array<unsigned char, 32> digest{};
  check(BCryptFinishHash(hash.handle, digest.data(), static_cast<ULONG>(digest.size()), 0));
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  result.reserve(64);
  for (auto byte : digest) {
    result.push_back(hex[byte >> 4]);
    result.push_back(hex[byte & 15]);
  }
  return result;
}

std::wstring buildWindowsTargetName(const std::string &app, const std::string &service,
                                    const std::string &key) {
  validateIdentity(app, service, key);
  const auto target = "rnssecurestore://v0/" + hashNamespaceComponent(app) + "/" +
                      hashNamespaceComponent(service) + "/" + hashNamespaceComponent(key);
  return {target.begin(), target.end()};
}
} // namespace rnssecurestore
