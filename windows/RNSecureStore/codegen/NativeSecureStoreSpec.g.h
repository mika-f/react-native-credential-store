
/*
 * This file is auto-generated from a NativeModule spec file in js.
 *
 * This is a C++ Spec class that should be used with MakeTurboModuleProvider to register native modules
 * in a way that also verifies at compile time that the native module matches the interface required
 * by the TurboModule JS spec.
 */
#pragma once
// clang-format off


#include <NativeModules.h>
#include <tuple>

namespace RNSecureStoreCodegen {

struct SecureStoreSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(std::string, std::string, std::string, std::string, Promise<void>) noexcept>{0, L"setItem"},
      Method<void(std::string, std::string, std::string, Promise<std::optional<std::string>>) noexcept>{1, L"getItem"},
      Method<void(std::string, std::string, std::string, Promise<void>) noexcept>{2, L"removeItem"},
      Method<void(std::string, std::string, std::string, Promise<bool>) noexcept>{3, L"hasItem"},
      Method<void(Promise<std::string>) noexcept>{4, L"getDefaultApplicationId"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, SecureStoreSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
          0,
          "setItem",
          "    REACT_METHOD(setItem) void setItem(std::string key, std::string value, std::string service, std::string applicationId, ::React::ReactPromise<void> &&result) noexcept { /* implementation */ }\n"
          "    REACT_METHOD(setItem) static void setItem(std::string key, std::string value, std::string service, std::string applicationId, ::React::ReactPromise<void> &&result) noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          1,
          "getItem",
          "    REACT_METHOD(getItem) void getItem(std::string key, std::string service, std::string applicationId, ::React::ReactPromise<std::optional<std::string>> &&result) noexcept { /* implementation */ }\n"
          "    REACT_METHOD(getItem) static void getItem(std::string key, std::string service, std::string applicationId, ::React::ReactPromise<std::optional<std::string>> &&result) noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          2,
          "removeItem",
          "    REACT_METHOD(removeItem) void removeItem(std::string key, std::string service, std::string applicationId, ::React::ReactPromise<void> &&result) noexcept { /* implementation */ }\n"
          "    REACT_METHOD(removeItem) static void removeItem(std::string key, std::string service, std::string applicationId, ::React::ReactPromise<void> &&result) noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          3,
          "hasItem",
          "    REACT_METHOD(hasItem) void hasItem(std::string key, std::string service, std::string applicationId, ::React::ReactPromise<bool> &&result) noexcept { /* implementation */ }\n"
          "    REACT_METHOD(hasItem) static void hasItem(std::string key, std::string service, std::string applicationId, ::React::ReactPromise<bool> &&result) noexcept { /* implementation */ }\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          4,
          "getDefaultApplicationId",
          "    REACT_METHOD(getDefaultApplicationId) void getDefaultApplicationId(::React::ReactPromise<std::string> &&result) noexcept { /* implementation */ }\n"
          "    REACT_METHOD(getDefaultApplicationId) static void getDefaultApplicationId(::React::ReactPromise<std::string> &&result) noexcept { /* implementation */ }\n");
  }
};

} // namespace RNSecureStoreCodegen
