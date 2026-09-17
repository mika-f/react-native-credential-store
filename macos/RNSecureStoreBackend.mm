#import "RNSecureStoreBackend.h"
#import "RNSecureStoreNamespace.h"
#import <Security/Security.h>
#include <mutex>

// The legacy macOS Keychain implementation can deadlock when multiple threads
// enter SecItemUpdate simultaneously. Share this lock across module instances
// (including multiple React runtimes), not just within one dispatch queue.
static std::mutex RNSecureStoreKeychainMutex;

static void RNSecureStoreStatusError(OSStatus status, NSError **error) {
  if (!error)
    return;
  NSString *code = (status == errSecAuthFailed || status == errSecInteractionNotAllowed ||
                    status == errSecMissingEntitlement)
                       ? @"E_ACCESS_DENIED"
                       : @"E_PLATFORM";
  *error = RNSecureStoreMakeError(code, @(status));
}

static NSMutableDictionary *RNSecureStoreQuery(NSString *key, NSString *service, NSString *app,
                                               NSError **error) {
  NSArray *values = @[ app ?: @"", service ?: @"", key ?: @"" ];
  NSArray *codes = @[ @"E_INVALID_APPLICATION_ID", @"E_INVALID_SERVICE", @"E_INVALID_KEY" ];
  for (NSUInteger i = 0; i < values.count; i++) {
    if (![values[i] isKindOfClass:NSString.class] || [values[i] length] == 0) {
      if (error)
        *error = RNSecureStoreMakeError(codes[i], nil);
      return nil;
    }
    if (!RNSecureStoreUTF8(values[i], error))
      return nil;
  }
  NSString *name = RNSecureStoreServiceNamespace(app, service, error);
  if (!name)
    return nil;
  return [@{
    (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
    (__bridge id)kSecAttrService : name,
    (__bridge id)kSecAttrAccount : key,
    (__bridge id)kSecAttrSynchronizable : @NO
  } mutableCopy];
}

@implementation RNSecureStoreBackend
+ (NSString *)defaultApplicationId {
  static NSString *identity;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    NSBundle *bundle = NSBundle.mainBundle;
    identity = bundle.bundleIdentifier;
    if (!identity.length) {
      NSString *metadata = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
      if (![metadata isKindOfClass:NSString.class] || !metadata.length)
        metadata = bundle.executablePath.lastPathComponent;
      if (!metadata.length)
        metadata = NSProcessInfo.processInfo.processName;
      identity = [@"local." stringByAppendingString:metadata.length ? metadata : @"application"];
    }
  });
  return identity;
}

+ (BOOL)setItem:(NSString *)key
            value:(NSString *)value
          service:(NSString *)service
    applicationId:(NSString *)applicationId
            error:(NSError **)error {
  const std::lock_guard<std::mutex> lock(RNSecureStoreKeychainMutex);
  NSMutableDictionary *query = RNSecureStoreQuery(key, service, applicationId, error);
  if (!query)
    return NO;
  NSData *bytes = RNSecureStoreUTF8(value, error);
  if (!bytes)
    return NO;
  if (bytes.length > 2560) {
    if (error)
      *error = RNSecureStoreMakeError(@"E_VALUE_TOO_LARGE", nil);
    return NO;
  }
  NSMutableDictionary *item = [query mutableCopy];
  item[(__bridge id)kSecValueData] = bytes;
  OSStatus status = SecItemAdd((__bridge CFDictionaryRef)item, nullptr);
  if (status == errSecDuplicateItem) {
    status = SecItemUpdate((__bridge CFDictionaryRef)query,
                           (__bridge CFDictionaryRef) @{(__bridge id)kSecValueData : bytes});
    // Another process may have deleted the item between add and update.
    if (status == errSecItemNotFound)
      status = SecItemAdd((__bridge CFDictionaryRef)item, nullptr);
  }
  if (status == errSecSuccess)
    return YES;
  RNSecureStoreStatusError(status, error);
  return NO;
}

+ (NSString *)getItem:(NSString *)key
              service:(NSString *)service
        applicationId:(NSString *)applicationId
                error:(NSError **)error {
  const std::lock_guard<std::mutex> lock(RNSecureStoreKeychainMutex);
  NSMutableDictionary *query = RNSecureStoreQuery(key, service, applicationId, error);
  if (!query)
    return nil;
  query[(__bridge id)kSecReturnData] = @YES;
  query[(__bridge id)kSecMatchLimit] = (__bridge id)kSecMatchLimitOne;
  CFTypeRef result = nullptr;
  OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &result);
  id data = CFBridgingRelease(result);
  if (status == errSecItemNotFound)
    return nil;
  if (status != errSecSuccess) {
    RNSecureStoreStatusError(status, error);
    return nil;
  }
  NSString *value = [data isKindOfClass:NSData.class]
                        ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]
                        : nil;
  if (!value && error)
    *error = RNSecureStoreMakeError(@"E_ENCODING", nil);
  return value;
}

+ (BOOL)removeItem:(NSString *)key
           service:(NSString *)service
     applicationId:(NSString *)applicationId
             error:(NSError **)error {
  const std::lock_guard<std::mutex> lock(RNSecureStoreKeychainMutex);
  NSMutableDictionary *query = RNSecureStoreQuery(key, service, applicationId, error);
  if (!query)
    return NO;
  OSStatus status = SecItemDelete((__bridge CFDictionaryRef)query);
  if (status == errSecSuccess || status == errSecItemNotFound)
    return YES;
  RNSecureStoreStatusError(status, error);
  return NO;
}

+ (BOOL)hasItem:(NSString *)key
          service:(NSString *)service
    applicationId:(NSString *)applicationId
            error:(NSError **)error {
  const std::lock_guard<std::mutex> lock(RNSecureStoreKeychainMutex);
  NSMutableDictionary *query = RNSecureStoreQuery(key, service, applicationId, error);
  if (!query)
    return NO;
  query[(__bridge id)kSecMatchLimit] = (__bridge id)kSecMatchLimitOne;
  // No return-data flag: do not materialize the secret to answer existence.
  OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, nullptr);
  if (status == errSecSuccess)
    return YES;
  if (status != errSecItemNotFound)
    RNSecureStoreStatusError(status, error);
  return NO;
}
@end
