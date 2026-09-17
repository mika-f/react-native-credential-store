#pragma once
#include "pch.h"
#include <optional>
#include "codegen/NativeSecureStoreSpec.g.h"

namespace winrt::RNSecureStore {
REACT_MODULE(RNSecureStore)
struct RNSecureStore {
  using ModuleSpec = RNSecureStoreCodegen::SecureStoreSpec;
  REACT_METHOD(setItem)
  void setItem(std::string key, std::string value, std::string service, std::string applicationId,
               Microsoft::ReactNative::ReactPromise<void> promise) noexcept;
  REACT_METHOD(getItem)
  void getItem(std::string key, std::string service, std::string applicationId,
               Microsoft::ReactNative::ReactPromise<std::optional<std::string>> promise) noexcept;
  REACT_METHOD(removeItem)
  void removeItem(std::string key, std::string service, std::string applicationId,
                  Microsoft::ReactNative::ReactPromise<void> promise) noexcept;
  REACT_METHOD(hasItem)
  void hasItem(std::string key, std::string service, std::string applicationId,
               Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;
  REACT_METHOD(getDefaultApplicationId)
  void getDefaultApplicationId(Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept;
};
} // namespace winrt::RNSecureStore
