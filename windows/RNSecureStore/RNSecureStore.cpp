#include "pch.h"
#include "RNSecureStore.h"
#include "CredentialStore.h"
#include "Namespace.h"
#include <type_traits>
#include <winrt/Windows.Foundation.h>

namespace winrt::RNSecureStore {
using namespace Microsoft::ReactNative;
namespace {
template <typename T, typename Operation>
winrt::fire_and_forget Run(ReactPromise<T> promise, Operation operation) {
  try {
    co_await winrt::resume_background();
    if constexpr (std::is_void_v<T>) {
      operation();
      promise.Resolve();
    } else
      promise.Resolve(operation());
  } catch (const rnssecurestore::Error &error) {
    ReactError result;
    result.Code = error.code;
    result.Message = "Secure credential store operation failed.";
    if (error.hasNativeCode)
      result.UserInfo["nativeCode"] = static_cast<int64_t>(error.nativeCode);
    promise.Reject(result);
  } catch (...) {
    ReactError result;
    result.Code = "E_UNKNOWN";
    result.Message = "Secure credential store operation failed.";
    promise.Reject(result);
  }
}
} // namespace
void RNSecureStore::setItem(std::string key, std::string value, std::string service,
                            std::string app, ReactPromise<void> promise) noexcept {
  Run(promise, [key = std::move(key), value = std::move(value), service = std::move(service),
                app = std::move(app)] { rnssecurestore::setItem(key, value, service, app); });
}
void RNSecureStore::getItem(std::string key, std::string service, std::string app,
                            ReactPromise<std::optional<std::string>> promise) noexcept {
  Run(promise, [key = std::move(key), service = std::move(service), app = std::move(app)] {
    return rnssecurestore::getItem(key, service, app);
  });
}
void RNSecureStore::removeItem(std::string key, std::string service, std::string app,
                               ReactPromise<void> promise) noexcept {
  Run(promise, [key = std::move(key), service = std::move(service), app = std::move(app)] {
    rnssecurestore::removeItem(key, service, app);
  });
}
void RNSecureStore::hasItem(std::string key, std::string service, std::string app,
                            ReactPromise<bool> promise) noexcept {
  Run(promise, [key = std::move(key), service = std::move(service), app = std::move(app)] {
    return rnssecurestore::hasItem(key, service, app);
  });
}
void RNSecureStore::getDefaultApplicationId(ReactPromise<std::string> promise) noexcept {
  Run(promise, [] { return rnssecurestore::defaultApplicationId(); });
}
} // namespace winrt::RNSecureStore
