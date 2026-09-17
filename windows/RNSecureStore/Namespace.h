#pragma once
#include <stdexcept>
#include <string>
#include <utility>

namespace rnssecurestore {
struct Error : std::runtime_error {
  std::string code;
  unsigned long nativeCode;
  bool hasNativeCode;
  explicit Error(std::string value)
      : std::runtime_error("Secure credential store operation failed."), code(std::move(value)),
        nativeCode(0), hasNativeCode(false) {}
  Error(std::string value, unsigned long native) : Error(std::move(value)) {
    nativeCode = native;
    hasNativeCode = true;
  }
};
void validateUtf8(const std::string &value);
void validateIdentity(const std::string &applicationId, const std::string &service,
                      const std::string &key);
std::string hashNamespaceComponent(const std::string &value);
std::wstring buildWindowsTargetName(const std::string &applicationId, const std::string &service,
                                    const std::string &key);
} // namespace rnssecurestore
