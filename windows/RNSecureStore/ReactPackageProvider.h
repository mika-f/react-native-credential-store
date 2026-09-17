#pragma once
#include "ReactPackageProvider.g.h"

namespace winrt::RNSecureStore::implementation {
struct ReactPackageProvider : ReactPackageProviderT<ReactPackageProvider> {
  void CreatePackage(Microsoft::ReactNative::IReactPackageBuilder const &builder) noexcept;
};
} // namespace winrt::RNSecureStore::implementation
namespace winrt::RNSecureStore::factory_implementation {
struct ReactPackageProvider
    : ReactPackageProviderT<ReactPackageProvider, implementation::ReactPackageProvider> {};
} // namespace winrt::RNSecureStore::factory_implementation
