#include "CredentialStore.h"
#include "Namespace.h"
#define NOMINMAX
#include <windows.h>
#include <appmodel.h>
#include <wincred.h>
#include <memory>
#include <vector>

namespace rnssecurestore {
namespace {
[[noreturn]] void platformError(DWORD code) {
  throw Error(code == ERROR_ACCESS_DENIED ? "E_ACCESS_DENIED" : "E_PLATFORM", code);
}
struct CredentialDeleter {
  void operator()(CREDENTIALW *credential) const noexcept {
    if (credential) {
      if (credential->CredentialBlob)
        SecureZeroMemory(credential->CredentialBlob, credential->CredentialBlobSize);
      CredFree(credential);
    }
  }
};
using Credential = std::unique_ptr<CREDENTIALW, CredentialDeleter>;
Credential read(const std::wstring &target) {
  PCREDENTIALW raw = nullptr;
  if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &raw)) {
    const DWORD code = GetLastError();
    if (code == ERROR_NOT_FOUND)
      return {};
    platformError(code);
  }
  return Credential(raw);
}
std::string utf8(const std::wstring &value) {
  if (value.empty())
    return {};
  int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                 static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
  if (!size)
    throw Error("E_ENCODING", GetLastError());
  std::string result(size, '\0');
  if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                           static_cast<int>(value.size()), result.data(), size, nullptr, nullptr))
    throw Error("E_ENCODING", GetLastError());
  return result;
}
std::wstring executablePath() {
  std::vector<wchar_t> buffer(32768);
  DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if (!size || size >= buffer.size())
    platformError(size ? ERROR_INSUFFICIENT_BUFFER : GetLastError());
  return {buffer.data(), size};
}
std::wstring metadataIdentity(const std::wstring &path) {
  DWORD unused = 0;
  DWORD size = GetFileVersionInfoSizeW(path.c_str(), &unused);
  if (!size)
    return {};
  std::vector<unsigned char> data(size);
  if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data()))
    return {};
  struct Translation {
    WORD language;
    WORD codepage;
  };
  Translation *translations = nullptr;
  UINT length = 0;
  if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation",
                      reinterpret_cast<void **>(&translations), &length))
    return {};
  for (UINT i = 0; i < length / sizeof(Translation); i++) {
    auto field = [&](const wchar_t *name) -> std::wstring {
      wchar_t query[128]{};
      swprintf_s(query, L"\\StringFileInfo\\%04x%04x\\%s", translations[i].language,
                 translations[i].codepage, name);
      wchar_t *text = nullptr;
      UINT count = 0;
      if (!VerQueryValueW(data.data(), query, reinterpret_cast<void **>(&text), &count) || !text ||
          count <= 1)
        return {};
      return std::wstring(text, count - 1);
    };
    auto company = field(L"CompanyName");
    auto name = field(L"InternalName");
    if (name.empty())
      name = field(L"ProductName");
    if (!company.empty() && !name.empty()) {
      // Length-prefix the fields to avoid delimiter collisions. No version data.
      return L"metadata." + std::to_wstring(company.size()) + L":" + company + L":" + name;
    }
  }
  return {};
}
} // namespace

std::string defaultApplicationId() {
  static const std::string identity = [] {
    UINT32 size = 0;
    LONG status = GetCurrentPackageFamilyName(&size, nullptr);
    if (status == ERROR_INSUFFICIENT_BUFFER) {
      std::vector<wchar_t> family(size);
      status = GetCurrentPackageFamilyName(&size, family.data());
      if (status == ERROR_SUCCESS)
        return utf8(family.data());
    }
    // Only unpackaged applications use metadata/fallback identities.
    if (status != APPMODEL_ERROR_NO_PACKAGE)
      platformError(static_cast<DWORD>(status));
    const auto path = executablePath();
    const auto metadata = metadataIdentity(path);
    if (!metadata.empty())
      return utf8(metadata);
    auto name = path.substr(path.find_last_of(L"\\/") + 1);
    auto dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos)
      name.resize(dot);
    return "local." + utf8(name.empty() ? L"application" : name);
  }();
  return identity;
}

void setItem(const std::string &key, const std::string &value, const std::string &service,
             const std::string &app) {
  auto target = buildWindowsTargetName(app, service, key);
  validateUtf8(value);
  if (value.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE)
    throw Error("E_VALUE_TOO_LARGE");
  struct SecretBuffer {
    std::vector<unsigned char> bytes;
    ~SecretBuffer() {
      if (!bytes.empty())
        SecureZeroMemory(bytes.data(), bytes.size());
    }
  } secret{{value.begin(), value.end()}};
  CREDENTIALW credential{};
  credential.Type = CRED_TYPE_GENERIC;
  credential.TargetName = target.data();
  credential.CredentialBlobSize = static_cast<DWORD>(secret.bytes.size());
  credential.CredentialBlob = secret.bytes.empty() ? nullptr : secret.bytes.data();
  credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
  if (!CredWriteW(&credential, 0))
    platformError(GetLastError());
}

std::optional<std::string> getItem(const std::string &key, const std::string &service,
                                   const std::string &app) {
  auto credential = read(buildWindowsTargetName(app, service, key));
  if (!credential)
    return std::nullopt;
  if (!credential->CredentialBlobSize)
    return std::string{};
  if (!credential->CredentialBlob)
    throw Error("E_ENCODING");
  std::string value(reinterpret_cast<const char *>(credential->CredentialBlob),
                    credential->CredentialBlobSize);
  validateUtf8(value);
  return value;
}

void removeItem(const std::string &key, const std::string &service, const std::string &app) {
  auto target = buildWindowsTargetName(app, service, key);
  if (!CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0)) {
    DWORD code = GetLastError();
    if (code != ERROR_NOT_FOUND)
      platformError(code);
  }
}

bool hasItem(const std::string &key, const std::string &service, const std::string &app) {
  return static_cast<bool>(read(buildWindowsTargetName(app, service, key)));
}
} // namespace rnssecurestore
