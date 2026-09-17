#include "pch.h"
#include "ReactPackageProvider.h"
#if __has_include("ReactPackageProvider.g.cpp")
#include "ReactPackageProvider.g.cpp"
#endif
#include "RNSecureStore.h"

namespace winrt::RNSecureStore::implementation {
void ReactPackageProvider::CreatePackage(
    Microsoft::ReactNative::IReactPackageBuilder const &builder) noexcept {
  Microsoft::ReactNative::AddAttributedModules(builder, true);
}
} // namespace winrt::RNSecureStore::implementation
